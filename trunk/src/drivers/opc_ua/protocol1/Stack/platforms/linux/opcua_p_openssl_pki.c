/* Copyright (c) 1996-2016, OPC Foundation. All rights reserved.

   The source code in this file is covered under a dual-license scenario:
     - RCL: for OPC Foundation members in good-standing
     - GPL V2: everybody else

   RCL license terms accompanied with this source code. See http://opcfoundation.org/License/RCL/1.00/

   GNU General Public License as published by the Free Software Foundation;
   version 2 of the License are accompanied with this source code. See http://opcfoundation.org/License/GPLv2

   This source code is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

/* UA platform definitions */
#include <opcua_p_internal.h>
#include <opcua_p_memory.h>
#include <opcua_p_string.h>

#if OPCUA_REQUIRE_OPENSSL


/* System Headers */
#include <openssl/pem.h>
#include <openssl/x509_vfy.h>
#include <openssl/x509v3.h>
#include <sys/types.h>
#include <dirent.h>



/* own headers */
#include <opcua_p_openssl_pki.h>

#define MAX_PATH 512

/* This functions filters out all hidden files and
 * files that don't have *.der extension.
 */
static int certificate_filter_der(const struct dirent *entry)
{
    char *pszFind;

    /* ignore hidden files */
    if (entry->d_name[0] == '.') return 0;

    /* check file extension */
    pszFind = strrchr(entry->d_name, '.');
    if (pszFind == 0) return 0;
    pszFind++;
    if (OpcUa_P_String_strnicmp(pszFind, "der", (OpcUa_UInt32)-1) == 0)
        return 1;

    return 0;
}

/** Certificate filter function for scandir.
 * This functions filters out all hidden files and
 * files that don't have *.crl extension.
 */
static int certificate_filter_crl(const struct dirent *entry)
{
    char *pszFind;

    /* ignore hidden files */
    if (entry->d_name[0] == '.') return 0;

    /* check file extension */
    pszFind = strrchr(entry->d_name, '.');
    if (pszFind == 0) return 0;
    pszFind++;
    if (OpcUa_P_String_strnicmp(pszFind, "crl", (OpcUa_UInt32)-1) == 0)
        return 1;

    return 0;
}

static
OpcUa_StatusCode OpcUa_P_OpenSSL_BuildFullPath( /*  in */ char*         a_pPath,
                                                /*  in */ char*         a_pFileName,
                                                /*  in */ unsigned int  a_uiFullPathBufferLength,
                                                /* out */ char*         a_pFullPath)
{
    unsigned int uiPathLength;
    unsigned int uiFileLength;

    OpcUa_ReturnErrorIfArgumentNull(a_pPath);
    OpcUa_ReturnErrorIfArgumentNull(a_pFileName);
    OpcUa_ReturnErrorIfArgumentNull(a_pFullPath);

    uiPathLength = (unsigned int)strlen(a_pPath);
    uiFileLength = (unsigned int)strlen(a_pFileName);

    if((uiPathLength + uiFileLength + 2) > a_uiFullPathBufferLength)
    {
        return OpcUa_BadInvalidArgument;
    }

    strncpy(a_pFullPath, a_pPath, uiPathLength + 1);
    strncat(a_pFullPath, "/", 1);
    strncat(a_pFullPath, a_pFileName, uiFileLength);

    return OpcUa_Good;
}

/*============================================================================
 * verify_callback
 *===========================================================================*/
static OpcUa_Int OpcUa_P_OpenSSL_CertificateStore_Verify_Callback(int a_ok, X509_STORE_CTX* a_pStore)
{
    OpcUa_P_OpenSSL_CertificateStore_Config*    pCertificateStoreCfg;

    pCertificateStoreCfg = X509_STORE_CTX_get_app_data(a_pStore);
    if(a_ok == 0)
    {
        /* certificate not ok */
        char    buf[256];
        X509*   err_cert;
        int     err;
        int     depth;

        err_cert = X509_STORE_CTX_get_current_cert(a_pStore);
        err      = X509_STORE_CTX_get_error(a_pStore);
        depth    = X509_STORE_CTX_get_error_depth(a_pStore);

        X509_NAME_oneline(X509_get_subject_name(err_cert), buf, 256);
        OpcUa_Trace(OPCUA_TRACE_LEVEL_WARNING, "\nverify error:\n\tnum=%d:%s\n\tdepth=%d\n\t%s\n", err, X509_verify_cert_error_string(err), depth, buf);

        X509_NAME_oneline(X509_get_issuer_name(a_pStore->current_cert), buf, 256);
        OpcUa_Trace(OPCUA_TRACE_LEVEL_WARNING, "\tissuer=%s\n", buf);

        switch (err)
        {
            case X509_V_ERR_UNABLE_TO_GET_CRL:
                if (pCertificateStoreCfg->Flags & OPCUA_P_PKI_OPENSSL_SUPPRESS_CRL_NOT_FOUND_ERROR)
                    a_ok = 1;
                break;

            case X509_V_ERR_CRL_NOT_YET_VALID:
            case X509_V_ERR_CRL_HAS_EXPIRED:
                if (pCertificateStoreCfg->Flags & OPCUA_P_PKI_OPENSSL_SUPPRESS_CRL_VALIDITY_PERIOD_CHECK)
                    a_ok = 1;
                break;

            case X509_V_ERR_CERT_NOT_YET_VALID:
            case X509_V_ERR_CERT_HAS_EXPIRED:
                if (pCertificateStoreCfg->Flags & OPCUA_P_PKI_OPENSSL_SUPPRESS_CERT_VALIDITY_PERIOD_CHECK)
                    a_ok = 1;
                break;
        }
    }

    return a_ok;
}

/*============================================================================
 * OpcUa_P_OpenSSL_CertificateStore_Open
 *===========================================================================*/
OpcUa_StatusCode OpcUa_P_OpenSSL_PKI_OpenCertificateStore(
    OpcUa_PKIProvider*          a_pProvider,
    OpcUa_Void**                a_ppCertificateStore)           /* type depends on store implementation */
{
    OpcUa_P_OpenSSL_CertificateStore_Config*    pCertificateStoreCfg;
    X509_STORE*         pStore;
    X509_LOOKUP*        pLookup;
    char                CertFile[MAX_PATH];
    struct dirent **dirlist = NULL;
    int numCertificates = 0, i;

OpcUa_InitializeStatus(OpcUa_Module_P_OpenSSL, "PKI_OpenCertificateStore");

    OpcUa_ReturnErrorIfArgumentNull(a_pProvider);
    OpcUa_ReturnErrorIfArgumentNull(a_pProvider->Handle);
    OpcUa_ReturnErrorIfArgumentNull(a_ppCertificateStore);

    *a_ppCertificateStore = OpcUa_Null;

    pCertificateStoreCfg = (OpcUa_P_OpenSSL_CertificateStore_Config*)a_pProvider->Handle;

    if(!(*a_ppCertificateStore = pStore = X509_STORE_new()))
    {
        OpcUa_GotoErrorWithStatus(OpcUa_Bad);
    }

    X509_STORE_set_verify_cb_func(pStore, OpcUa_P_OpenSSL_CertificateStore_Verify_Callback);

    if(pCertificateStoreCfg->Flags & OPCUA_P_PKI_OPENSSL_USE_DEFAULT_CERT_CRL_LOOKUP_METHOD)
    {
        if(X509_STORE_set_default_paths(pStore) != 1)
        {
            OpcUa_Trace(OPCUA_TRACE_LEVEL_WARNING, "error at X509_STORE_set_default_paths!\n");
            OpcUa_GotoErrorWithStatus(OpcUa_Bad);
        }
    }

    if(!(pCertificateStoreCfg->Flags & OPCUA_P_PKI_OPENSSL_DONT_ADD_TRUST_LIST_TO_ROOT_CERTIFICATES))
    {
        if(pCertificateStoreCfg->CertificateTrustListLocation == OpcUa_Null || pCertificateStoreCfg->CertificateTrustListLocation[0] == '\0')
        {
            OpcUa_GotoErrorWithStatus(OpcUa_Bad);
        }

        /* how to search for certificate & CRLs */
        if(!(pLookup = X509_STORE_add_lookup(pStore, X509_LOOKUP_file())))
        {
            OpcUa_GotoErrorWithStatus(OpcUa_Bad);
        }

        /* how to search for certificate & CRLs */
        numCertificates = scandir(pCertificateStoreCfg->CertificateTrustListLocation, &dirlist, certificate_filter_der, alphasort);
        for (i=0; i<numCertificates; i++)
        {
            uStatus = OpcUa_P_OpenSSL_BuildFullPath(pCertificateStoreCfg->CertificateTrustListLocation, dirlist[i]->d_name, MAX_PATH, CertFile);
            OpcUa_GotoErrorIfBad(uStatus);

            /* add CACertificate lookup */
            if(X509_LOOKUP_load_file(pLookup, CertFile, X509_FILETYPE_ASN1) != 1) /*DER encoded*/
            {
                OpcUa_Trace(OPCUA_TRACE_LEVEL_WARNING, "error at X509_LOOKUP_load_file: skipping %s\n", CertFile);
            }
        }
        for (i=0; i<numCertificates; i++)
        {
            free(dirlist[i]);
        }
        free(dirlist);
        dirlist = NULL;
    }

    if(pCertificateStoreCfg->Flags & OPCUA_P_PKI_OPENSSL_ADD_UNTRUSTED_LIST_TO_ROOT_CERTIFICATES)
    {
        if(pCertificateStoreCfg->CertificateUntrustedListLocation == OpcUa_Null || pCertificateStoreCfg->CertificateUntrustedListLocation[0] == '\0')
        {
            OpcUa_GotoErrorWithStatus(OpcUa_Bad);
        }

        if(pCertificateStoreCfg->Flags & OPCUA_P_PKI_OPENSSL_UNTRUSTED_LIST_IS_INDEX)
        {
            /* how to search for certificate */
            if(!(pLookup = X509_STORE_add_lookup(pStore, X509_LOOKUP_hash_dir())))
            {
                OpcUa_GotoErrorWithStatus(OpcUa_Bad);
            }

            /* add hash lookup */
            if(X509_LOOKUP_add_dir(pLookup, pCertificateStoreCfg->CertificateUntrustedListLocation, X509_FILETYPE_ASN1) != 1) /*DER encoded*/
            {
                OpcUa_Trace(OPCUA_TRACE_LEVEL_WARNING, "error at X509_LOOKUP_add_dir!\n");
                OpcUa_GotoErrorWithStatus(OpcUa_Bad);
            }
        }
        else
        {
            /* how to search for certificate & CRLs */
            if(!(pLookup = X509_STORE_add_lookup(pStore, X509_LOOKUP_file())))
            {
                OpcUa_GotoErrorWithStatus(OpcUa_Bad);
            }

            numCertificates = scandir(pCertificateStoreCfg->CertificateUntrustedListLocation, &dirlist, certificate_filter_der, alphasort);
            for (i=0; i<numCertificates; i++)
            {
                uStatus = OpcUa_P_OpenSSL_BuildFullPath(pCertificateStoreCfg->CertificateUntrustedListLocation, dirlist[i]->d_name, MAX_PATH, CertFile);
                OpcUa_GotoErrorIfBad(uStatus);

                /* add CACertificate lookup */
                if(X509_LOOKUP_load_file(pLookup, CertFile, X509_FILETYPE_ASN1) != 1) /*DER encoded*/
                {
                    OpcUa_Trace(OPCUA_TRACE_LEVEL_WARNING, "error at X509_LOOKUP_load_file: skipping %s\n", CertFile);
                }
            }
            for (i=0; i<numCertificates; i++)
            {
                free(dirlist[i]);
            }
            free(dirlist);
            dirlist = NULL;
        }
    }

    if(pCertificateStoreCfg->Flags & OPCUA_P_PKI_OPENSSL_CHECK_REVOCATION_ALL)
    {
        if(pCertificateStoreCfg->CertificateRevocationListLocation == OpcUa_Null || pCertificateStoreCfg->CertificateRevocationListLocation[0] == '\0')
        {
            OpcUa_GotoErrorWithStatus(OpcUa_Bad);
        }

        if(pCertificateStoreCfg->Flags & OPCUA_P_PKI_OPENSSL_REVOCATION_LIST_IS_INDEX)
        {
            /* how to search for certificate & CRLs */
            if(!(pLookup = X509_STORE_add_lookup(pStore, X509_LOOKUP_hash_dir())))
            {
                OpcUa_GotoErrorWithStatus(OpcUa_Bad);
            }

            /* add CRL lookup */
            if(X509_LOOKUP_add_dir(pLookup, pCertificateStoreCfg->CertificateRevocationListLocation, X509_FILETYPE_PEM) != 1) /*PEM encoded*/
            {
                OpcUa_Trace(OPCUA_TRACE_LEVEL_WARNING, "error at X509_LOOKUP_add_dir!\n");
                OpcUa_GotoErrorWithStatus(OpcUa_Bad);
            }
        }
        else if(pCertificateStoreCfg->Flags & OPCUA_P_PKI_OPENSSL_REVOCATION_LIST_IS_CONCATENATED_PEM_FILE)
        {
            /* how to search for certificate & CRLs */
            if(!(pLookup = X509_STORE_add_lookup(pStore, X509_LOOKUP_file())))
            {
                OpcUa_GotoErrorWithStatus(OpcUa_Bad);
            }

            /* add CRL lookup */
            if(X509_load_crl_file(pLookup, pCertificateStoreCfg->CertificateRevocationListLocation, X509_FILETYPE_PEM) != 1) /*PEM encoded*/
            {
                OpcUa_Trace(OPCUA_TRACE_LEVEL_WARNING, "error at X509_load_crl_file!\n");
            }
        }
        else
        {
            /* how to search for certificate & CRLs */
            if(!(pLookup = X509_STORE_add_lookup(pStore, X509_LOOKUP_file())))
            {
                OpcUa_GotoErrorWithStatus(OpcUa_Bad);
            }

            numCertificates = scandir(pCertificateStoreCfg->CertificateRevocationListLocation, &dirlist, certificate_filter_crl, alphasort);
            for (i=0; i<numCertificates; i++)
            {
                uStatus = OpcUa_P_OpenSSL_BuildFullPath(pCertificateStoreCfg->CertificateRevocationListLocation, dirlist[i]->d_name, MAX_PATH, CertFile);
                OpcUa_GotoErrorIfBad(uStatus);

                if(X509_load_crl_file(pLookup, CertFile, X509_FILETYPE_PEM) != 1) /*PEM encoded*/
                {
                    OpcUa_Trace(OPCUA_TRACE_LEVEL_WARNING, "error at X509_load_crl_file: skipping %s\n", CertFile);
                }
            }
            for (i=0; i<numCertificates; i++)
            {
                free(dirlist[i]);
            }
            free(dirlist);
            dirlist = NULL;
        }

        if((pCertificateStoreCfg->Flags & OPCUA_P_PKI_OPENSSL_CHECK_REVOCATION_ALL) == OPCUA_P_PKI_OPENSSL_CHECK_REVOCATION_ALL)
        {
            /* set the flags of the store so that CRLs are consulted */
            if(X509_STORE_set_flags(pStore, X509_V_FLAG_CRL_CHECK | X509_V_FLAG_CRL_CHECK_ALL) != 1)
            {
                OpcUa_GotoErrorWithStatus(OpcUa_Bad);
            }
        }
        else if(pCertificateStoreCfg->Flags & OPCUA_P_PKI_OPENSSL_CHECK_REVOCATION_ONLY_LEAF)
        {
            /* set the flags of the store so that CRLs are consulted */
            if(X509_STORE_set_flags(pStore, X509_V_FLAG_CRL_CHECK) != 1)
            {
                OpcUa_GotoErrorWithStatus(OpcUa_Bad);
            }
        }
    }

    if(pCertificateStoreCfg->Flags & OPCUA_P_PKI_OPENSSL_CHECK_SELF_SIGNED_SIGNATURE)
    {
        /* set the flags of the store so that CRLs are consulted */
        if(X509_STORE_set_flags(pStore, X509_V_FLAG_CHECK_SS_SIGNATURE) != 1)
        {
            OpcUa_GotoErrorWithStatus(OpcUa_Bad);
        }
    }

    if(pCertificateStoreCfg->Flags & OPCUA_P_PKI_OPENSSL_ALLOW_PROXY_CERTIFICATES)
    {
        /* set the flags of the store so that CRLs are consulted */
        if(X509_STORE_set_flags(pStore, X509_V_FLAG_ALLOW_PROXY_CERTS) != 1)
        {
            OpcUa_GotoErrorWithStatus(OpcUa_Bad);
        }
    }

OpcUa_ReturnStatusCode;
OpcUa_BeginErrorHandling;

    if(dirlist != NULL)
    {
        for (i=0; i<numCertificates; i++)
        {
            free(dirlist[i]);
        }
        free(dirlist);
    }

    if(*a_ppCertificateStore != OpcUa_Null)
    {
        X509_STORE_free((X509_STORE*)*a_ppCertificateStore);
        *a_ppCertificateStore = OpcUa_Null;
    }

OpcUa_FinishErrorHandling;
}

/*============================================================================
 * OpcUa_P_OpenSSL_CertificateStore_Close
 *===========================================================================*/
OpcUa_StatusCode OpcUa_P_OpenSSL_PKI_CloseCertificateStore(
    OpcUa_PKIProvider*          a_pProvider,
    OpcUa_Void**                a_ppCertificateStore) /* type depends on store implementation */
{
OpcUa_InitializeStatus(OpcUa_Module_P_OpenSSL, "PKI_CloseCertificateStore");

    OpcUa_ReferenceParameter(a_pProvider);

    if(*a_ppCertificateStore != OpcUa_Null)
    {
        X509_STORE_free((X509_STORE*)*a_ppCertificateStore);
        *a_ppCertificateStore = OpcUa_Null;
    }

OpcUa_ReturnStatusCode;
OpcUa_BeginErrorHandling;
OpcUa_FinishErrorHandling;
}

/*============================================================================
 * OpcUa_P_OpenSSL_PKI_ValidateCertificate
 *===========================================================================*/
OpcUa_StatusCode OpcUa_P_OpenSSL_PKI_ValidateCertificate(
    OpcUa_PKIProvider*          a_pProvider,
    OpcUa_ByteString*           a_pCertificate,
    OpcUa_Void*                 a_pCertificateStore,
    OpcUa_Int*                  a_pValidationCode /* Validation return codes from OpenSSL */
    )
{
    OpcUa_P_OpenSSL_CertificateStore_Config*    pCertificateStoreCfg;

    const unsigned char* p;

    X509*               pX509Certificate        = OpcUa_Null;
    STACK_OF(X509)*     pX509Chain              = OpcUa_Null;
    X509_STORE_CTX*     verify_ctx              = OpcUa_Null;    /* holds data used during verification process */
    char                CertFile[MAX_PATH];
    struct dirent **dirlist = NULL;
    int numCertificates = 0, i;

OpcUa_InitializeStatus(OpcUa_Module_P_OpenSSL, "PKI_ValidateCertificate");

    OpcUa_ReturnErrorIfArgumentNull(a_pProvider);
    OpcUa_ReturnErrorIfArgumentNull(a_pProvider->Handle);
    OpcUa_ReturnErrorIfArgumentNull(a_pCertificate);
    OpcUa_ReturnErrorIfArgumentNull(a_pCertificateStore);
    OpcUa_ReturnErrorIfArgumentNull(a_pValidationCode);

    pCertificateStoreCfg = (OpcUa_P_OpenSSL_CertificateStore_Config*)a_pProvider->Handle;

    /* convert DER encoded bytestring certificate to openssl X509 certificate */
    p = a_pCertificate->Data;
    if(!(pX509Certificate = d2i_X509((X509**)OpcUa_Null, &p, a_pCertificate->Length)))
    {
        OpcUa_GotoErrorWithStatus(OpcUa_Bad);
    }

    while(p < a_pCertificate->Data + a_pCertificate->Length)
    {
        X509* pX509AddCertificate;
        if(!(pX509AddCertificate = d2i_X509((X509**)OpcUa_Null, &p, a_pCertificate->Data + a_pCertificate->Length - p)))
        {
            OpcUa_GotoErrorWithStatus(OpcUa_Bad);
        }
        if(pX509Chain == NULL)
        {
            pX509Chain = sk_X509_new_null();
            OpcUa_GotoErrorIfAllocFailed(pX509Chain);
        }
        if(!sk_X509_push(pX509Chain, pX509AddCertificate))
        {
            X509_free(pX509AddCertificate);
            OpcUa_GotoErrorWithStatus(OpcUa_Bad);
        }
    }

    /* create verification context and initialize it */
    if(!(verify_ctx = X509_STORE_CTX_new()))
    {
        OpcUa_GotoErrorWithStatus(OpcUa_Bad);
    }

#if (OPENSSL_VERSION_NUMBER > 0x00907000L)
    if(X509_STORE_CTX_init(verify_ctx, (X509_STORE*)a_pCertificateStore, pX509Certificate, pX509Chain) != 1)
    {
        OpcUa_GotoErrorWithStatus(OpcUa_Bad);
    }
#else
    X509_STORE_CTX_init(verify_ctx, (X509_STORE*)a_pCertificateStore, pX509Certificate, pX509Chain);
#endif

    if(X509_STORE_CTX_set_app_data(verify_ctx, pCertificateStoreCfg) != 1)
    {
        OpcUa_GotoErrorWithStatus(OpcUa_Bad);
    }

    if((pCertificateStoreCfg->Flags & OPCUA_P_PKI_OPENSSL_CHECK_REVOCATION_ALL) == OPCUA_P_PKI_OPENSSL_CHECK_REVOCATION_ALL_EXCEPT_SELF_SIGNED
        && !verify_ctx->check_issued(verify_ctx, pX509Certificate, pX509Certificate))
    {
        /* set the flags of the store so that CRLs are consulted */
        X509_STORE_CTX_set_flags(verify_ctx, X509_V_FLAG_CRL_CHECK | X509_V_FLAG_CRL_CHECK_ALL);
    }

    /* verify the certificate */
    *a_pValidationCode = X509_V_OK;
    if(X509_verify_cert(verify_ctx) <= 0)
    {
        *a_pValidationCode = verify_ctx->error;
        switch(verify_ctx->error)
        {
        case X509_V_ERR_CERT_HAS_EXPIRED:
        case X509_V_ERR_CERT_NOT_YET_VALID:
        case X509_V_ERR_CRL_NOT_YET_VALID:
        case X509_V_ERR_CRL_HAS_EXPIRED:
        case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
        case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
        case X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD:
        case X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD:
            {
                uStatus = OpcUa_BadCertificateTimeInvalid;
                break;
            }
        case X509_V_ERR_CERT_REVOKED:
            {
                uStatus = OpcUa_BadCertificateRevoked;
                break;
            }
        case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
        case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
        case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
            {
                uStatus = OpcUa_BadCertificateUntrusted;
                break;
            }
        case X509_V_ERR_CERT_SIGNATURE_FAILURE:
            {
                uStatus = OpcUa_BadSecurityChecksFailed;
                break;
            }
        default:
            {
                uStatus = OpcUa_BadCertificateInvalid;
            }
        }
        OpcUa_GotoErrorIfBad(uStatus);
    }

    if(pCertificateStoreCfg->Flags & OPCUA_P_PKI_OPENSSL_REQUIRE_CHAIN_CERTIFICATE_IN_TRUST_LIST)
    {
        FILE*            pCertificateFile;
        X509*            pTrustCert;
        STACK_OF(X509)*  chain;
        int              trusted, n;

        chain = X509_STORE_CTX_get_chain(verify_ctx);
        trusted = 0;
        if(pCertificateStoreCfg->CertificateTrustListLocation == NULL || pCertificateStoreCfg->CertificateTrustListLocation[0] == '\0')
        {
            uStatus = OpcUa_Bad;
            OpcUa_GotoErrorIfBad(uStatus);
        }

        numCertificates = scandir(pCertificateStoreCfg->CertificateTrustListLocation, &dirlist, certificate_filter_der, alphasort);
        for (i=0; i<numCertificates; i++)
        {
            uStatus = OpcUa_P_OpenSSL_BuildFullPath(pCertificateStoreCfg->CertificateTrustListLocation, dirlist[i]->d_name, MAX_PATH, CertFile);
            OpcUa_GotoErrorIfBad(uStatus);

            /* read DER certificates */
            pCertificateFile = fopen(CertFile, "r");
            if(pCertificateFile == OpcUa_Null)
            {
                continue; /* ignore access errors */
            }

            pTrustCert = d2i_X509_fp(pCertificateFile, (X509**)OpcUa_Null);
            fclose(pCertificateFile);
            if(pTrustCert == OpcUa_Null)
            {
                continue; /* ignore parse errors */
            }

            for(n = 0; n < sk_X509_num(chain); n++)
            {
                if (X509_cmp(sk_X509_value(chain, n), pTrustCert) == 0)
                    break;
            }

            X509_free(pTrustCert);
            if(n < sk_X509_num(chain))
            {
                trusted = 1;
                break;
            }
        }
        for (i=0; i<numCertificates; i++)
        {
            free(dirlist[i]);
        }
        free(dirlist);
        dirlist = NULL;

        if(!trusted)
        {
            uStatus = OpcUa_BadCertificateUntrusted;
            OpcUa_GotoErrorIfBad(uStatus);
        }
    }

    X509_STORE_CTX_free(verify_ctx);
    X509_free(pX509Certificate);
    if(pX509Chain != OpcUa_Null)
    {
        sk_X509_pop_free(pX509Chain, X509_free);
    }

OpcUa_ReturnStatusCode;
OpcUa_BeginErrorHandling;

    if(dirlist != NULL)
    {
        for (i=0; i<numCertificates; i++)
        {
            free(dirlist[i]);
        }
        free(dirlist);
    }

    if(verify_ctx != OpcUa_Null)
    {
        X509_STORE_CTX_free(verify_ctx);
    }

    if(pX509Certificate != OpcUa_Null)
    {
        X509_free(pX509Certificate);
    }

    if(pX509Chain != OpcUa_Null)
    {
        sk_X509_pop_free(pX509Chain, X509_free);
    }

OpcUa_FinishErrorHandling;
}

/*============================================================================
 * OpcUa_P_OpenSSL_PKI_SaveCertificate
 *===========================================================================*/
/*
    ToDo:   Create Access to OpenSSL certificate store
            => Only API to In-Memory-Store is available for version 0.9.8x
            => Wait until Directory- and/or File-Store is available
*/
OpcUa_StatusCode OpcUa_P_OpenSSL_PKI_SaveCertificate(
    OpcUa_PKIProvider*          a_pProvider,
    OpcUa_ByteString*           a_pCertificate,
    OpcUa_Void*                 a_pCertificateStore,
    OpcUa_Void*                 a_pSaveHandle)      /* Index or number within store/destination filepath */
{
    X509*                                       pX509Certificate        = OpcUa_Null;
    FILE*                                       pCertificateFile        = OpcUa_Null;

    const unsigned char*                        p;

    OpcUa_UInt32                                i;

OpcUa_InitializeStatus(OpcUa_Module_P_OpenSSL, "PKI_SaveCertificate");


    OpcUa_ReturnErrorIfArgumentNull(a_pProvider);
    OpcUa_ReturnErrorIfArgumentNull(a_pProvider->Handle);
    OpcUa_ReturnErrorIfArgumentNull(a_pCertificate);
    OpcUa_ReturnErrorIfArgumentNull(a_pCertificateStore);
    OpcUa_ReturnErrorIfArgumentNull(a_pSaveHandle);

    /* save DER certificate */
    pCertificateFile = fopen((const char*)a_pSaveHandle, "w");

    /* check for valid file handle */
    OpcUa_GotoErrorIfTrue((pCertificateFile == OpcUa_Null), OpcUa_BadInvalidArgument);

    /* convert openssl X509 certificate to DER encoded bytestring certificate */
    p = a_pCertificate->Data;
    while (p < a_pCertificate->Data + a_pCertificate->Length)
    {
        if(!(pX509Certificate = d2i_X509((X509**)OpcUa_Null, &p, a_pCertificate->Data + a_pCertificate->Length - p)))
        {
            fclose(pCertificateFile);
            uStatus = OpcUa_Bad;
            OpcUa_GotoErrorIfBad(uStatus);
        }

        i = i2d_X509_fp(pCertificateFile, pX509Certificate);

        if(i < 1)
        {
            fclose(pCertificateFile);
            uStatus =  OpcUa_Bad;
            OpcUa_GotoErrorIfBad(uStatus);
        }

        X509_free(pX509Certificate);
        pX509Certificate = OpcUa_Null;
    }

    if(fclose(pCertificateFile) != 0)
    {
        uStatus =  OpcUa_Bad;
        OpcUa_GotoErrorIfBad(uStatus);
    }

OpcUa_ReturnStatusCode;
OpcUa_BeginErrorHandling;

    if (pX509Certificate != OpcUa_Null)
    {
        X509_free(pX509Certificate);
    }

OpcUa_FinishErrorHandling;
}

/*============================================================================
 * OpcUa_P_OpenSSL_CertificateStore_Certificate_Load
 *===========================================================================*/
/*
    ToDo:   Create Access to OpenSSL certificate store
            => Only API to In-Memory-Store is available for version 0.9.8x
            => Wait until Directory- and/or File-Store is available
*/
OpcUa_StatusCode OpcUa_P_OpenSSL_PKI_LoadCertificate(
    OpcUa_PKIProvider*          a_pProvider,
    OpcUa_Void*                 a_pLoadHandle,
    OpcUa_Void*                 a_pCertificateStore,
    OpcUa_ByteString*           a_pCertificate)
{
    OpcUa_Byte*     buf                 = OpcUa_Null;
    OpcUa_Byte*     p                   = OpcUa_Null;
    FILE*           pCertificateFile    = OpcUa_Null;
    X509*           pTmpCert            = OpcUa_Null;

    OpcUa_InitializeStatus(OpcUa_Module_P_OpenSSL, "PKI_LoadCertificate");

    OpcUa_ReturnErrorIfArgumentNull(a_pProvider);
    OpcUa_ReturnErrorIfArgumentNull(a_pProvider->Handle);
    OpcUa_ReturnErrorIfArgumentNull(a_pLoadHandle);
    OpcUa_ReturnErrorIfArgumentNull(a_pCertificateStore);
    OpcUa_ReturnErrorIfArgumentNull(a_pCertificate);

    /* read DER certificates */
    pCertificateFile = fopen((const char*)a_pLoadHandle, "r");

    /* check for valid file handle */
    OpcUa_GotoErrorIfTrue((pCertificateFile == OpcUa_Null), OpcUa_BadInvalidArgument);

    if(!(pTmpCert = d2i_X509_fp(pCertificateFile, (X509**)OpcUa_Null)))
    {
        uStatus = OpcUa_Bad;
        OpcUa_GotoErrorIfBad(uStatus);
    }

    a_pCertificate->Length = i2d_X509(pTmpCert, NULL);
    buf = (OpcUa_Byte*)OpcUa_P_Memory_Alloc(a_pCertificate->Length);
    OpcUa_GotoErrorIfAllocFailed(buf);
    p = buf;
    for (;;)
    {
        i2d_X509(pTmpCert, &p);
        X509_free(pTmpCert);
        if(!(pTmpCert = d2i_X509_fp(pCertificateFile, (X509**)OpcUa_Null)))
        {
            break;
        }
        p = OpcUa_P_Memory_ReAlloc(buf, a_pCertificate->Length + i2d_X509(pTmpCert, NULL));
        OpcUa_GotoErrorIfAllocFailed(p);
        buf = p;
        p = buf + a_pCertificate->Length;
        a_pCertificate->Length += i2d_X509(pTmpCert, NULL);
    }

    if(fclose(pCertificateFile) != 0)
    {
        pCertificateFile = OpcUa_Null;
        uStatus =  OpcUa_Bad;
        OpcUa_GotoErrorIfBad(uStatus);
    }

    a_pCertificate->Data = buf;

OpcUa_ReturnStatusCode;
OpcUa_BeginErrorHandling;

    if(pCertificateFile != OpcUa_Null)
    {
        fclose(pCertificateFile);
    }

    if(pTmpCert != OpcUa_Null)
    {
        X509_free(pTmpCert);
    }

    if(buf != OpcUa_Null)
    {
        OpcUa_P_Memory_Free(buf);
    }

OpcUa_FinishErrorHandling;
}

/**
  @brief Extracts data from a certificate store object.

  @param pCertificate          [in] The certificate to examine.
  @param pIssuer               [out, optional] The issuer name of the certificate.
  @param pSubject              [out, optional] The subject name of the certificate.
  @param pSubjectUri           [out, optional] The subject's URI of the certificate.
  @param pSubjectIP            [out, optional] The subject's IP of the certificate.
  @param pSubjectDNS           [out, optional] The subject's DNS name of the certificate.
  @param pCertThumbprint       [out, optional] The thumbprint of the certificate.
  @param pSubjectHash          [out, optional] The hash code of the certificate.
  @param pCertRawLength        [out, optional] The length of the DER encoded data.
                               can be smaller than the total length of pCertificate in case of chain certificate or garbage follow.
*/
OpcUa_StatusCode OpcUa_P_OpenSSL_PKI_ExtractCertificateData(
    OpcUa_ByteString*           a_pCertificate,
    OpcUa_ByteString*           a_pIssuer,
    OpcUa_ByteString*           a_pSubject,
    OpcUa_ByteString*           a_pSubjectUri,
    OpcUa_ByteString*           a_pSubjectIP,
    OpcUa_ByteString*           a_pSubjectDNS,
    OpcUa_ByteString*           a_pCertThumbprint,
    OpcUa_UInt32*               a_pSubjectHash,
    OpcUa_UInt32*               a_pCertRawLength)
{
    X509*                       pX509Cert = OpcUa_Null;
    char*                       pName = OpcUa_Null;
    GENERAL_NAMES*              pNames = OpcUa_Null;
    const unsigned char*        p;

OpcUa_InitializeStatus(OpcUa_Module_P_OpenSSL, "PKI_ExtractCertificateData");

    OpcUa_ReturnErrorIfArgumentNull(a_pCertificate);

    if(a_pIssuer != OpcUa_Null)
    {
        a_pIssuer->Data = OpcUa_Null;
        a_pIssuer->Length = 0;
    }

    if(a_pSubject != OpcUa_Null)
    {
        a_pSubject->Data = OpcUa_Null;
        a_pSubject->Length = 0;
    }

    if(a_pSubjectUri != OpcUa_Null)
    {
        a_pSubjectUri->Data = OpcUa_Null;
        a_pSubjectUri->Length = 0;
    }

    if(a_pSubjectIP != OpcUa_Null)
    {
        a_pSubjectIP->Data = OpcUa_Null;
        a_pSubjectIP->Length = 0;
    }

    if(a_pSubjectDNS != OpcUa_Null)
    {
        a_pSubjectDNS->Data = OpcUa_Null;
        a_pSubjectDNS->Length = 0;
    }

    if(a_pCertThumbprint != OpcUa_Null)
    {
        a_pCertThumbprint->Data = OpcUa_Null;
        a_pCertThumbprint->Length = 0;
    }

    if(a_pSubjectHash != OpcUa_Null)
    {
        *a_pSubjectHash = 0;
    }

    if(a_pCertRawLength != OpcUa_Null)
    {
        *a_pCertRawLength = 0;
    }

    /* convert openssl X509 certificate to DER encoded bytestring certificate */
    p = a_pCertificate->Data;
    if(!(pX509Cert = d2i_X509((X509**)OpcUa_Null, &p, a_pCertificate->Length)))
    {
        uStatus = OpcUa_Bad;
        OpcUa_GotoErrorIfBad(uStatus);
    }

    if(a_pIssuer != OpcUa_Null)
    {
        pName = X509_NAME_oneline(X509_get_issuer_name(pX509Cert), NULL, 0);
        OpcUa_GotoErrorIfAllocFailed(pName);
        a_pIssuer->Length = strlen(pName)+1;
        a_pIssuer->Data = (OpcUa_Byte*)OpcUa_P_Memory_Alloc(a_pIssuer->Length*sizeof(OpcUa_Byte));
        OpcUa_GotoErrorIfAllocFailed(a_pIssuer->Data);
        uStatus = OpcUa_P_Memory_MemCpy(a_pIssuer->Data, a_pIssuer->Length, pName, a_pIssuer->Length);
        OpcUa_GotoErrorIfBad(uStatus);
        OPENSSL_free(pName);
        pName = OpcUa_Null;
    }

    if(a_pSubject != OpcUa_Null)
    {
        pName = X509_NAME_oneline(X509_get_subject_name(pX509Cert), NULL, 0);
        OpcUa_GotoErrorIfAllocFailed(pName);
        a_pSubject->Length = strlen(pName)+1;
        a_pSubject->Data = (OpcUa_Byte*)OpcUa_P_Memory_Alloc(a_pSubject->Length*sizeof(OpcUa_Byte));
        OpcUa_GotoErrorIfAllocFailed(a_pSubject->Data);
        uStatus = OpcUa_P_Memory_MemCpy(a_pSubject->Data, a_pSubject->Length, pName, a_pSubject->Length);
        OpcUa_GotoErrorIfBad(uStatus);
        OPENSSL_free(pName);
        pName = OpcUa_Null;
    }

    if(a_pSubjectUri != OpcUa_Null || a_pSubjectIP != OpcUa_Null || a_pSubjectDNS != OpcUa_Null)
    {
        pNames = X509_get_ext_d2i(pX509Cert, NID_subject_alt_name, OpcUa_Null, OpcUa_Null);
        if (pNames != OpcUa_Null)
        {
            int num;
            for (num = 0; num < sk_GENERAL_NAME_num(pNames); num++)
            {
                GENERAL_NAME *value = sk_GENERAL_NAME_value(pNames, num);
                switch (value->type)
                {
                case GEN_URI:
                    if (a_pSubjectUri != OpcUa_Null && a_pSubjectUri->Data == OpcUa_Null)
                    {
                        a_pSubjectUri->Length = value->d.ia5->length+1;
                        a_pSubjectUri->Data = (OpcUa_Byte*)OpcUa_P_Memory_Alloc(a_pSubjectUri->Length*sizeof(OpcUa_Byte));
                        OpcUa_GotoErrorIfAllocFailed(a_pSubjectUri->Data);
                        uStatus = OpcUa_P_Memory_MemCpy(a_pSubjectUri->Data, a_pSubjectUri->Length, value->d.ia5->data, a_pSubjectUri->Length);
                        OpcUa_GotoErrorIfBad(uStatus);
                    }
                break;

                case GEN_IPADD:
                    if (a_pSubjectIP != OpcUa_Null && a_pSubjectIP->Data == OpcUa_Null)
                    {
                        a_pSubjectIP->Length = value->d.ip->length;
                        a_pSubjectIP->Data = (OpcUa_Byte*)OpcUa_P_Memory_Alloc(a_pSubjectIP->Length*sizeof(OpcUa_Byte));
                        OpcUa_GotoErrorIfAllocFailed(a_pSubjectIP->Data);
                        uStatus = OpcUa_P_Memory_MemCpy(a_pSubjectIP->Data, a_pSubjectIP->Length, value->d.ip->data, a_pSubjectIP->Length);
                        OpcUa_GotoErrorIfBad(uStatus);
                    }
                break;

                case GEN_DNS:
                    if (a_pSubjectDNS != OpcUa_Null && a_pSubjectDNS->Data == OpcUa_Null)
                    {
                        a_pSubjectDNS->Length = value->d.ia5->length+1;
                        a_pSubjectDNS->Data = (OpcUa_Byte*)OpcUa_P_Memory_Alloc(a_pSubjectDNS->Length*sizeof(OpcUa_Byte));
                        OpcUa_GotoErrorIfAllocFailed(a_pSubjectDNS->Data);
                        uStatus = OpcUa_P_Memory_MemCpy(a_pSubjectDNS->Data, a_pSubjectDNS->Length, value->d.ia5->data, a_pSubjectDNS->Length);
                        OpcUa_GotoErrorIfBad(uStatus);
                    }
                break;
                }
            }
            sk_GENERAL_NAME_pop_free(pNames, GENERAL_NAME_free);
            pNames = OpcUa_Null;
        }
    }

    if(a_pCertThumbprint != OpcUa_Null)
    {
        /* update pX509Cert->sha1_hash */
        X509_check_purpose(pX509Cert, -1, 0);
        a_pCertThumbprint->Length = sizeof(pX509Cert->sha1_hash);
        a_pCertThumbprint->Data = (OpcUa_Byte*)OpcUa_P_Memory_Alloc(a_pCertThumbprint->Length*sizeof(OpcUa_Byte));
        OpcUa_GotoErrorIfAllocFailed(a_pCertThumbprint->Data);
        uStatus = OpcUa_P_Memory_MemCpy(a_pCertThumbprint->Data, a_pCertThumbprint->Length, pX509Cert->sha1_hash, a_pCertThumbprint->Length);
        OpcUa_GotoErrorIfBad(uStatus);
    }

    if(a_pSubjectHash != OpcUa_Null)
    {
        *a_pSubjectHash = X509_NAME_hash(X509_get_subject_name(pX509Cert));
    }

    if(a_pCertRawLength != OpcUa_Null)
    {
        *a_pCertRawLength = (OpcUa_UInt32)(p - a_pCertificate->Data);
    }

    X509_free(pX509Cert);

OpcUa_ReturnStatusCode;
OpcUa_BeginErrorHandling;

    if(a_pIssuer != OpcUa_Null && a_pIssuer->Data != OpcUa_Null)
    {
        OpcUa_P_Memory_Free(a_pIssuer->Data);
        a_pIssuer->Data = OpcUa_Null;
        a_pIssuer->Length = 0;
    }

    if(a_pSubject != OpcUa_Null && a_pSubject->Data != OpcUa_Null)
    {
        OpcUa_P_Memory_Free(a_pSubject->Data);
        a_pSubject->Data = OpcUa_Null;
        a_pSubject->Length = 0;
    }

    if(a_pSubjectUri != OpcUa_Null && a_pSubjectUri->Data != OpcUa_Null)
    {
        OpcUa_P_Memory_Free(a_pSubjectUri->Data);
        a_pSubjectUri->Data = OpcUa_Null;
        a_pSubjectUri->Length = 0;
    }

    if(a_pSubjectIP != OpcUa_Null && a_pSubjectIP->Data != OpcUa_Null)
    {
        OpcUa_P_Memory_Free(a_pSubjectIP->Data);
        a_pSubjectIP->Data = OpcUa_Null;
        a_pSubjectIP->Length = 0;
    }

    if(a_pSubjectDNS != OpcUa_Null && a_pSubjectDNS->Data != OpcUa_Null)
    {
        OpcUa_P_Memory_Free(a_pSubjectDNS->Data);
        a_pSubjectDNS->Data = OpcUa_Null;
        a_pSubjectDNS->Length = 0;
    }

    if(a_pCertThumbprint != OpcUa_Null && a_pCertThumbprint->Data != OpcUa_Null)
    {
        OpcUa_P_Memory_Free(a_pCertThumbprint->Data);
        a_pCertThumbprint->Data = OpcUa_Null;
        a_pCertThumbprint->Length = 0;
    }

    if (pName != OpcUa_Null)
    {
        OPENSSL_free(pName);
    }

    if (pNames != OpcUa_Null)
    {
        sk_GENERAL_NAME_pop_free(pNames, GENERAL_NAME_free);
    }

    if(pX509Cert != OpcUa_Null)
    {
        X509_free(pX509Cert);
    }

OpcUa_FinishErrorHandling;
}

#endif /* OPCUA_REQUIRE_OPENSSL */

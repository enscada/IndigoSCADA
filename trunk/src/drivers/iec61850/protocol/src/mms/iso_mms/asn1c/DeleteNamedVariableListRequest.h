/*
 * Generated by asn1c-0.9.21 (http://lionet.info/asn1c)
 * From ASN.1 module "MMS"
 * 	found in "../mms-extended.asn"
 * 	`asn1c -fskeletons-copy`
 */

#ifndef	_DeleteNamedVariableListRequest_H_
#define	_DeleteNamedVariableListRequest_H_


#include <asn_application.h>

/* Including external dependencies */
#include <INTEGER.h>
#include "Identifier.h"
#include <asn_SEQUENCE_OF.h>
#include <constr_SEQUENCE_OF.h>
#include <constr_SEQUENCE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
typedef enum DeleteNamedVariableListRequest__scopeOfDelete {
	DeleteNamedVariableListRequest__scopeOfDelete_specific	= 0,
	DeleteNamedVariableListRequest__scopeOfDelete_aaspecific	= 1,
	DeleteNamedVariableListRequest__scopeOfDelete_domain	= 2,
	DeleteNamedVariableListRequest__scopeOfDelete_vmd	= 3
} e_DeleteNamedVariableListRequest__scopeOfDelete;

/* Forward declarations */
struct ObjectName;

/* DeleteNamedVariableListRequest */
typedef struct DeleteNamedVariableListRequest {
	INTEGER_t	*scopeOfDelete	/* DEFAULT 0 */;
	struct DeleteNamedVariableListRequest__listOfVariableListName {
		A_SEQUENCE_OF(struct ObjectName) list;
		
		/* Context for parsing across buffer boundaries */
		asn_struct_ctx_t _asn_ctx;
	} *listOfVariableListName;
	Identifier_t	*domainName	/* OPTIONAL */;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} DeleteNamedVariableListRequest_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_DeleteNamedVariableListRequest;

#ifdef __cplusplus
}
#endif

/* Referred external types */
#include "ObjectName.h"

#endif	/* _DeleteNamedVariableListRequest_H_ */
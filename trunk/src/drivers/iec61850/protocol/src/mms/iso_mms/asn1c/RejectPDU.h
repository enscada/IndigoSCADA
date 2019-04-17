/*
 * Generated by asn1c-0.9.21 (http://lionet.info/asn1c)
 * From ASN.1 module "MMS"
 * 	found in "../mms-extended.asn"
 * 	`asn1c -fskeletons-copy`
 */

#ifndef	_RejectPDU_H_
#define	_RejectPDU_H_


#include <asn_application.h>

/* Including external dependencies */
#include "Unsigned32.h"
#include <NativeInteger.h>
#include <INTEGER.h>
#include <NULL.h>
#include <constr_CHOICE.h>
#include <constr_SEQUENCE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
typedef enum RejectPDU__rejectReason_PR {
	RejectPDU__rejectReason_PR_NOTHING,	/* No components present */
	RejectPDU__rejectReason_PR_confirmedRequestPDU,
	RejectPDU__rejectReason_PR_confirmedResponsePDU,
	RejectPDU__rejectReason_PR_confirmedErrorPDU,
	RejectPDU__rejectReason_PR_unconfirmedPDU,
	RejectPDU__rejectReason_PR_pduError,
	RejectPDU__rejectReason_PR_cancelRequestPDU,
	RejectPDU__rejectReason_PR_cancelResponsePDU,
	RejectPDU__rejectReason_PR_cancelErrorPDU,
	RejectPDU__rejectReason_PR_concludeRequestPDU,
	RejectPDU__rejectReason_PR_concludeResponsePDU,
	RejectPDU__rejectReason_PR_concludeErrorPDU
} RejectPDU__rejectReason_PR;
typedef enum RejectPDU__rejectReason__confirmedRequestPDU {
	RejectPDU__rejectReason__confirmedRequestPDU_other	= 0,
	RejectPDU__rejectReason__confirmedRequestPDU_unrecognizedService	= 1,
	RejectPDU__rejectReason__confirmedRequestPDU_unrecognizedModifier	= 2,
	RejectPDU__rejectReason__confirmedRequestPDU_invalidInvokeID	= 3,
	RejectPDU__rejectReason__confirmedRequestPDU_invalidArgument	= 4,
	RejectPDU__rejectReason__confirmedRequestPDU_invalidModifier	= 5,
	RejectPDU__rejectReason__confirmedRequestPDU_maxServOutstandingEexceeded	= 6,
	RejectPDU__rejectReason__confirmedRequestPDU_maxRecursionExceeded	= 8,
	RejectPDU__rejectReason__confirmedRequestPDU_valueOutOfRange	= 9
} e_RejectPDU__rejectReason__confirmedRequestPDU;
typedef enum RejectPDU__rejectReason__confirmedResponsePDU {
	RejectPDU__rejectReason__confirmedResponsePDU_other	= 0,
	RejectPDU__rejectReason__confirmedResponsePDU_unrecognizedService	= 1,
	RejectPDU__rejectReason__confirmedResponsePDU_invalidInvokeID	= 2,
	RejectPDU__rejectReason__confirmedResponsePDU_invalidResult	= 3,
	RejectPDU__rejectReason__confirmedResponsePDU_maxRecursionExceeded	= 5,
	RejectPDU__rejectReason__confirmedResponsePDU_valueOutOfRange	= 6
} e_RejectPDU__rejectReason__confirmedResponsePDU;
typedef enum RejectPDU__rejectReason__confirmedErrorPDU {
	RejectPDU__rejectReason__confirmedErrorPDU_other	= 0,
	RejectPDU__rejectReason__confirmedErrorPDU_unrecognizedService	= 1,
	RejectPDU__rejectReason__confirmedErrorPDU_invalidInvokeID	= 2,
	RejectPDU__rejectReason__confirmedErrorPDU_invalidServiceError	= 3,
	RejectPDU__rejectReason__confirmedErrorPDU_valueOutOfRange	= 4
} e_RejectPDU__rejectReason__confirmedErrorPDU;
typedef enum RejectPDU__rejectReason__unconfirmedPDU {
	RejectPDU__rejectReason__unconfirmedPDU_other	= 0,
	RejectPDU__rejectReason__unconfirmedPDU_unrecognizedService	= 1,
	RejectPDU__rejectReason__unconfirmedPDU_invalidArgument	= 2,
	RejectPDU__rejectReason__unconfirmedPDU_maxRecursionExceeded	= 3,
	RejectPDU__rejectReason__unconfirmedPDU_valueOutOfRange	= 4
} e_RejectPDU__rejectReason__unconfirmedPDU;
typedef enum RejectPDU__rejectReason__pduError {
	RejectPDU__rejectReason__pduError_unknownPduType	= 0,
	RejectPDU__rejectReason__pduError_invalidPdu	= 1,
	RejectPDU__rejectReason__pduError_illegalAcseMapping	= 2
} e_RejectPDU__rejectReason__pduError;
typedef enum RejectPDU__rejectReason__concludeRequestPDU {
	RejectPDU__rejectReason__concludeRequestPDU_other	= 0,
	RejectPDU__rejectReason__concludeRequestPDU_invalidArgument	= 1
} e_RejectPDU__rejectReason__concludeRequestPDU;
typedef enum RejectPDU__rejectReason__concludeResponsePDU {
	RejectPDU__rejectReason__concludeResponsePDU_other	= 0,
	RejectPDU__rejectReason__concludeResponsePDU_invalidResult	= 1
} e_RejectPDU__rejectReason__concludeResponsePDU;
typedef enum RejectPDU__rejectReason__concludeErrorPDU {
	RejectPDU__rejectReason__concludeErrorPDU_other	= 0,
	RejectPDU__rejectReason__concludeErrorPDU_invalidServiceError	= 1,
	RejectPDU__rejectReason__concludeErrorPDU_valueOutOfRange	= 2
} e_RejectPDU__rejectReason__concludeErrorPDU;

struct RejectPDU__rejectReason {
		RejectPDU__rejectReason_PR present;
		union RejectPDU__rejectReason_u {
			long	 confirmedRequestPDU;
			long	 confirmedResponsePDU;
			long	 confirmedErrorPDU;
			long	 unconfirmedPDU;
			INTEGER_t	 pduError;
			NULL_t	 cancelRequestPDU;
			NULL_t	 cancelResponsePDU;
			NULL_t	 cancelErrorPDU;
			long	 concludeRequestPDU;
			long	 concludeResponsePDU;
			long	 concludeErrorPDU;
		} choice;
		
		/* Context for parsing across buffer boundaries */
		asn_struct_ctx_t _asn_ctx;
	};

/* RejectPDU */
typedef struct RejectPDU {
	Unsigned32_t	*originalInvokeID	/* OPTIONAL */;
	struct RejectPDU__rejectReason rejectReason;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} RejectPDU_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_RejectPDU;

#ifdef __cplusplus
}
#endif

#endif	/* _RejectPDU_H_ */

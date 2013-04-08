/*
 * Generated by asn1c-0.9.21 (http://lionet.info/asn1c)
 * From ASN.1 module "ISO8823PRESENTATION"
 * 	found in "../isoPresentationLayer.asn"
 * 	`asn1c -fskeletons-copy`
 */

#ifndef	_Protocolversion_H_
#define	_Protocolversion_H_


#include <asn_application.h>

/* Including external dependencies */
#include <BIT_STRING.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
typedef enum Protocolversion {
	Protocolversion_version1	= 0
} e_Protocolversion;

/* Protocolversion */
typedef BIT_STRING_t	 Protocolversion_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_Protocolversion;
asn_struct_free_f Protocolversion_free;
asn_struct_print_f Protocolversion_print;
asn_constr_check_f Protocolversion_constraint;
ber_type_decoder_f Protocolversion_decode_ber;
der_type_encoder_f Protocolversion_encode_der;
xer_type_decoder_f Protocolversion_decode_xer;
xer_type_encoder_f Protocolversion_encode_xer;

#ifdef __cplusplus
}
#endif

#endif	/* _Protocolversion_H_ */

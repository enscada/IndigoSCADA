/*
 * Generated by asn1c-0.9.21 (http://lionet.info/asn1c)
 * From ASN.1 module "MMS"
 * 	found in "../mms-extended.asn"
 * 	`asn1c -fskeletons-copy`
 */

#include <asn_internal.h>

#include "GetNameListResponse.h"

static asn_TYPE_member_t asn_MBR_listOfIdentifier_2[] = {
	{ ATF_POINTER, 0, 0,
		(ASN_TAG_CLASS_UNIVERSAL | (26 << 2)),
		0,
		&asn_DEF_Identifier,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		""
		},
};
static ber_tlv_tag_t asn_DEF_listOfIdentifier_tags_2[] = {
	(ASN_TAG_CLASS_CONTEXT | (0 << 2)),
	(ASN_TAG_CLASS_UNIVERSAL | (16 << 2))
};
static asn_SET_OF_specifics_t asn_SPC_listOfIdentifier_specs_2 = {
	sizeof(struct GetNameListResponse__listOfIdentifier),
	offsetof(struct GetNameListResponse__listOfIdentifier, _asn_ctx),
	0,	/* XER encoding is XMLDelimitedItemList */
};
static /* Use -fall-defs-global to expose */
asn_TYPE_descriptor_t asn_DEF_listOfIdentifier_2 = {
	"listOfIdentifier",
	"listOfIdentifier",
	SEQUENCE_OF_free,
	SEQUENCE_OF_print,
	SEQUENCE_OF_constraint,
	SEQUENCE_OF_decode_ber,
	SEQUENCE_OF_encode_der,
	SEQUENCE_OF_decode_xer,
	SEQUENCE_OF_encode_xer,
	0, 0,	/* No PER support, use "-gen-PER" to enable */
	0,	/* Use generic outmost tag fetcher */
	asn_DEF_listOfIdentifier_tags_2,
	sizeof(asn_DEF_listOfIdentifier_tags_2)
		/sizeof(asn_DEF_listOfIdentifier_tags_2[0]) - 1, /* 1 */
	asn_DEF_listOfIdentifier_tags_2,	/* Same as above */
	sizeof(asn_DEF_listOfIdentifier_tags_2)
		/sizeof(asn_DEF_listOfIdentifier_tags_2[0]), /* 2 */
	0,	/* No PER visible constraints */
	asn_MBR_listOfIdentifier_2,
	1,	/* Single element */
	&asn_SPC_listOfIdentifier_specs_2	/* Additional specs */
};

static asn_TYPE_member_t asn_MBR_GetNameListResponse_1[] = {
	{ ATF_NOFLAGS, 0, offsetof(struct GetNameListResponse, listOfIdentifier),
		(ASN_TAG_CLASS_CONTEXT | (0 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_listOfIdentifier_2,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"listOfIdentifier"
		},
	{ ATF_POINTER, 1, offsetof(struct GetNameListResponse, moreFollows),
		(ASN_TAG_CLASS_CONTEXT | (1 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_BOOLEAN,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"moreFollows"
		},
};
static ber_tlv_tag_t asn_DEF_GetNameListResponse_tags_1[] = {
	(ASN_TAG_CLASS_UNIVERSAL | (16 << 2))
};
static asn_TYPE_tag2member_t asn_MAP_GetNameListResponse_tag2el_1[] = {
    { (ASN_TAG_CLASS_CONTEXT | (0 << 2)), 0, 0, 0 }, /* listOfIdentifier at 509 */
    { (ASN_TAG_CLASS_CONTEXT | (1 << 2)), 1, 0, 0 } /* moreFollows at 510 */
};
static asn_SEQUENCE_specifics_t asn_SPC_GetNameListResponse_specs_1 = {
	sizeof(struct GetNameListResponse),
	offsetof(struct GetNameListResponse, _asn_ctx),
	asn_MAP_GetNameListResponse_tag2el_1,
	2,	/* Count of tags in the map */
	0, 0, 0,	/* Optional elements (not needed) */
	-1,	/* Start extensions */
	-1	/* Stop extensions */
};
asn_TYPE_descriptor_t asn_DEF_GetNameListResponse = {
	"GetNameListResponse",
	"GetNameListResponse",
	SEQUENCE_free,
	SEQUENCE_print,
	SEQUENCE_constraint,
	SEQUENCE_decode_ber,
	SEQUENCE_encode_der,
	SEQUENCE_decode_xer,
	SEQUENCE_encode_xer,
	0, 0,	/* No PER support, use "-gen-PER" to enable */
	0,	/* Use generic outmost tag fetcher */
	asn_DEF_GetNameListResponse_tags_1,
	sizeof(asn_DEF_GetNameListResponse_tags_1)
		/sizeof(asn_DEF_GetNameListResponse_tags_1[0]), /* 1 */
	asn_DEF_GetNameListResponse_tags_1,	/* Same as above */
	sizeof(asn_DEF_GetNameListResponse_tags_1)
		/sizeof(asn_DEF_GetNameListResponse_tags_1[0]), /* 1 */
	0,	/* No PER visible constraints */
	asn_MBR_GetNameListResponse_1,
	2,	/* Elements count */
	&asn_SPC_GetNameListResponse_specs_1	/* Additional specs */
};


/*
 * Generated by asn1c-0.9.21 (http://lionet.info/asn1c)
 * From ASN.1 module "MMS"
 * 	found in "../mms-extended.asn"
 * 	`asn1c -fskeletons-copy`
 */

#include <asn_internal.h>

#include "AlternateAccessSelection.h"

static asn_TYPE_member_t asn_MBR_selectAccess_2[] = {
	{ ATF_NOFLAGS, 0, offsetof(struct AlternateAccessSelection__selectAccess, choice.component),
		(ASN_TAG_CLASS_CONTEXT | (1 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_Identifier,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"component"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct AlternateAccessSelection__selectAccess, choice.index),
		(ASN_TAG_CLASS_CONTEXT | (2 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_Unsigned32,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"index"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct AlternateAccessSelection__selectAccess, choice.indexRange),
		(ASN_TAG_CLASS_CONTEXT | (3 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_IndexRangeSeq,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"indexRange"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct AlternateAccessSelection__selectAccess, choice.allElements),
		(ASN_TAG_CLASS_CONTEXT | (4 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_NULL,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"allElements"
		},
};
static asn_TYPE_tag2member_t asn_MAP_selectAccess_tag2el_2[] = {
    { (ASN_TAG_CLASS_CONTEXT | (1 << 2)), 0, 0, 0 }, /* component at 582 */
    { (ASN_TAG_CLASS_CONTEXT | (2 << 2)), 1, 0, 0 }, /* index at 583 */
    { (ASN_TAG_CLASS_CONTEXT | (3 << 2)), 2, 0, 0 }, /* indexRange at 584 */
    { (ASN_TAG_CLASS_CONTEXT | (4 << 2)), 3, 0, 0 } /* allElements at 585 */
};
static asn_CHOICE_specifics_t asn_SPC_selectAccess_specs_2 = {
	sizeof(struct AlternateAccessSelection__selectAccess),
	offsetof(struct AlternateAccessSelection__selectAccess, _asn_ctx),
	offsetof(struct AlternateAccessSelection__selectAccess, present),
	sizeof(((struct AlternateAccessSelection__selectAccess *)0)->present),
	asn_MAP_selectAccess_tag2el_2,
	4,	/* Count of tags in the map */
	0,
	-1	/* Extensions start */
};
static /* Use -fall-defs-global to expose */
asn_TYPE_descriptor_t asn_DEF_selectAccess_2 = {
	"selectAccess",
	"selectAccess",
	CHOICE_free,
	CHOICE_print,
	CHOICE_constraint,
	CHOICE_decode_ber,
	CHOICE_encode_der,
	CHOICE_decode_xer,
	CHOICE_encode_xer,
	0, 0,	/* No PER support, use "-gen-PER" to enable */
	CHOICE_outmost_tag,
	0,	/* No effective tags (pointer) */
	0,	/* No effective tags (count) */
	0,	/* No tags (pointer) */
	0,	/* No tags (count) */
	0,	/* No PER visible constraints */
	asn_MBR_selectAccess_2,
	4,	/* Elements count */
	&asn_SPC_selectAccess_specs_2	/* Additional specs */
};

static asn_TYPE_member_t asn_MBR_AlternateAccessSelection_1[] = {
	{ ATF_NOFLAGS, 0, offsetof(struct AlternateAccessSelection, choice.selectAccess),
		-1 /* Ambiguous tag (CHOICE?) */,
		0,
		&asn_DEF_selectAccess_2,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"selectAccess"
		},
};
static asn_TYPE_tag2member_t asn_MAP_AlternateAccessSelection_tag2el_1[] = {
    { (ASN_TAG_CLASS_CONTEXT | (1 << 2)), 0, 0, 0 }, /* component at 582 */
    { (ASN_TAG_CLASS_CONTEXT | (2 << 2)), 0, 0, 0 }, /* index at 583 */
    { (ASN_TAG_CLASS_CONTEXT | (3 << 2)), 0, 0, 0 }, /* indexRange at 584 */
    { (ASN_TAG_CLASS_CONTEXT | (4 << 2)), 0, 0, 0 } /* allElements at 585 */
};
static asn_CHOICE_specifics_t asn_SPC_AlternateAccessSelection_specs_1 = {
	sizeof(struct AlternateAccessSelection),
	offsetof(struct AlternateAccessSelection, _asn_ctx),
	offsetof(struct AlternateAccessSelection, present),
	sizeof(((struct AlternateAccessSelection *)0)->present),
	asn_MAP_AlternateAccessSelection_tag2el_1,
	4,	/* Count of tags in the map */
	0,
	-1	/* Extensions start */
};
asn_TYPE_descriptor_t asn_DEF_AlternateAccessSelection = {
	"AlternateAccessSelection",
	"AlternateAccessSelection",
	CHOICE_free,
	CHOICE_print,
	CHOICE_constraint,
	CHOICE_decode_ber,
	CHOICE_encode_der,
	CHOICE_decode_xer,
	CHOICE_encode_xer,
	0, 0,	/* No PER support, use "-gen-PER" to enable */
	CHOICE_outmost_tag,
	0,	/* No effective tags (pointer) */
	0,	/* No effective tags (count) */
	0,	/* No tags (pointer) */
	0,	/* No tags (count) */
	0,	/* No PER visible constraints */
	asn_MBR_AlternateAccessSelection_1,
	1,	/* Elements count */
	&asn_SPC_AlternateAccessSelection_specs_1	/* Additional specs */
};

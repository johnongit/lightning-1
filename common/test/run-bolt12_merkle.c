#define SUPERVERBOSE printf
#include "config.h"
/* Needed before including bolt12_merkle.c: */
  #include <common/type_to_string.h>
  #include <stdio.h>
#include "../bolt12_merkle.c"
#include <assert.h>
#include <ccan/array_size/array_size.h>
#include <common/channel_type.h>
#include <common/features.h>
#include <common/setup.h>

/* Definition of n1 from the spec */
#include <wire/peer_wire.h>

/* AUTOGENERATED MOCKS START */
/* Generated stub for features_unsupported */
int features_unsupported(const struct feature_set *our_features UNNEEDED,
			 const u8 *their_features UNNEEDED,
			 enum feature_place p UNNEEDED)
{ fprintf(stderr, "features_unsupported called!\n"); abort(); }
/* Generated stub for fromwire_blinded_path */
struct blinded_path *fromwire_blinded_path(const tal_t *ctx UNNEEDED, const u8 **cursor UNNEEDED, size_t *plen UNNEEDED)
{ fprintf(stderr, "fromwire_blinded_path called!\n"); abort(); }
/* Generated stub for fromwire_channel_id */
bool fromwire_channel_id(const u8 **cursor UNNEEDED, size_t *max UNNEEDED,
			 struct channel_id *channel_id UNNEEDED)
{ fprintf(stderr, "fromwire_channel_id called!\n"); abort(); }
/* Generated stub for towire_blinded_path */
void towire_blinded_path(u8 **p UNNEEDED, const struct blinded_path *blinded_path UNNEEDED)
{ fprintf(stderr, "towire_blinded_path called!\n"); abort(); }
/* Generated stub for towire_channel_id */
void towire_channel_id(u8 **pptr UNNEEDED, const struct channel_id *channel_id UNNEEDED)
{ fprintf(stderr, "towire_channel_id called!\n"); abort(); }
/* AUTOGENERATED MOCKS END */

/* Contat several tal objects */
#define concat(p, ...) concat_((p), __VA_ARGS__, NULL)

static LAST_ARG_NULL void *concat_(const void *p, ...)
{
	va_list ap;
	size_t len = 0;
	u8 *ret = tal_arr(tmpctx, u8, len);

	va_start(ap, p);
	do {
		tal_resize(&ret, len + tal_bytelen(p));
		memcpy(ret + len, p, tal_bytelen(p));
		len += tal_bytelen(p);
	} while ((p = va_arg(ap, const void *)) != NULL);
	va_end(ap);
	return ret;
}

/* Hashes a tal object */
static struct sha256 *SHA256(const void *obj)
{
	struct sha256 *ret = tal(tmpctx, struct sha256);
	sha256(ret, obj, tal_bytelen(obj));
	return ret;
}

/* Concatenate these two in lesser, greater order. */
static u8 *ordered(const struct sha256 *a, const struct sha256 *b)
{
	u8 *ret = tal_arr(tmpctx, u8, sizeof(*a) + sizeof(*b));

	if (memcmp(a, b, sizeof(*a)) < 0) {
		memcpy(ret, a, sizeof(*a));
		memcpy(ret + sizeof(*a), b, sizeof(*b));
	} else {
		memcpy(ret, b, sizeof(*b));
		memcpy(ret + sizeof(*b), a, sizeof(*a));
	}
	return ret;
}

static u8 *tlv(u64 type, const void *contents, size_t len)
{
	u8 *ret = tal_arr(tmpctx, u8, 0);

	towire_bigsize(&ret, type);
	towire_bigsize(&ret, len);
	towire(&ret, contents, len);
	return ret;
}

/* BOLT-offers #12:
 * Thus we define H(`tag`,`msg`) as SHA256(SHA256(`tag`) ||
 * SHA256(`tag`) || `msg`) */

static struct sha256 *H(const void *tag, const void *msg)
{
	const struct sha256 *taghash = SHA256(tag);
	const u8 *full = concat(taghash, taghash, msg);
	struct sha256 *ret = SHA256(full);

	printf("test: H(tag=%s,msg=%s) -> SHA256(%s|%s|msg) -> %s\n",
	       tal_hex(tmpctx, tag), tal_hex(tmpctx, msg),
	       type_to_string(tmpctx, struct sha256, taghash),
	       type_to_string(tmpctx, struct sha256, taghash),
	       type_to_string(tmpctx, struct sha256, ret));
	return ret;
}

static void merkle_n1(const struct tlv_n1 *n1, struct sha256 *test_m)
{
	u8 *v;
	size_t len;
	struct tlv_n1 *tmp;

	/* Linearize to populate ->fields */
	v = tal_arr(tmpctx, u8, 0);
	towire_tlv_n1(&v, n1);

	len = tal_bytelen(v);
	tmp = fromwire_tlv_n1(tmpctx, cast_const2(const u8 **, &v), &len);
	assert(len == 0);

	merkle_tlv(tmp->fields, test_m);
}

/* As a bonus, you get the merkle-test.json by running:
 * common/test/run-bolt12_merkle | grep '^JSON:' | cut -d: -f2- | jq */
#define json_out(fmt, ...) printf("JSON: " fmt "\n" , ## __VA_ARGS__)

int main(int argc, char *argv[])
{
	struct sha256 *m, test_m, *leaf[6];
	const char *LnBranch, *LnAll, *LnLeaf;
	u8 *tlv1, *tlv2, *tlv3, *all;
	struct tlv_n1 *n1;
	char *fail;

	common_setup(argv[0]);
	/* Note: no nul term */
	LnBranch = tal_dup_arr(tmpctx, char, "LnBranch", strlen("LnBranch"), 0);
	LnLeaf = tal_dup_arr(tmpctx, char, "LnLeaf", strlen("LnLeaf"), 0);
	LnAll = tal_dup_arr(tmpctx, char, "LnAll", strlen("LnAll"), 0);

	/* Create the tlvs, as per example `n1` in spec */
	{
		u8 *v;
		struct short_channel_id scid;
		struct node_id nid;

		v = tal_arr(tmpctx, u8, 0);
		towire_tu64(&v, 1000);
		tlv1 = tlv(1, v, tal_bytelen(v));

		v = tal_arr(tmpctx, u8, 0);
		if (!mk_short_channel_id(&scid, 1, 2, 3))
			abort();
		towire_short_channel_id(&v, &scid);
		tlv2 = tlv(2, v, tal_bytelen(v));

		node_id_from_hexstr("0266e4598d1d3c415f572a8488830b60f7e744ed9235eb0b1ba93283b315c03518", strlen("0266e4598d1d3c415f572a8488830b60f7e744ed9235eb0b1ba93283b315c03518"), &nid);
		v = tal_arr(tmpctx, u8, 0);
		towire_node_id(&v, &nid);
		towire_amount_msat(&v, AMOUNT_MSAT(1));
		towire_amount_msat(&v, AMOUNT_MSAT(2));
		tlv3 = tlv(3, v, tal_bytelen(v));
	}

	json_out("[");

	json_out("{\"comment\": \"Simple n1 test, tlv1 = 1000\",");
	json_out("\"tlv\": \"n1\",");
	/* Simplest case, a single (msat) element. */
	all = tlv1;
	json_out("\"all-tlvs\": \"%s\",", tal_hex(tmpctx, all));
	json_out("\"leaves\": [");

	leaf[0] = H(LnBranch,
		    ordered(H(LnLeaf, tlv1),
			    H(concat(LnAll, all), tlv1)));
	json_out("{ \"H(`LnLeaf`,%s)\": \"%s\", \"H(`LnAll`|all-tlvs,tlv1)\": \"%s\", \"H(`LnBranch`,leaf+nonce)\": \"%s\" }",
		 tal_hex(tmpctx, tlv1),
		 type_to_string(tmpctx, struct sha256, H(LnLeaf, tlv1)),
		 type_to_string(tmpctx, struct sha256, H(concat(LnAll, all), tlv1)),
		 type_to_string(tmpctx, struct sha256, leaf[0]));
	json_out("],");

	m = leaf[0];

	json_out("\"branches\": [],");
	json_out("\"merkle\": \"%s\"",
		 type_to_string(tmpctx, struct sha256, m));
	json_out("},");

	printf("n1 = %s, merkle = %s\n",
	       tal_hex(tmpctx, all),
	       type_to_string(tmpctx, struct sha256, m));

	/* Create, linearize (populates ->fields) */
	n1 = tlv_n1_new(tmpctx);
	n1->tlv1 = tal(n1, u64);
	*n1->tlv1 = 1000;
	merkle_n1(n1, &test_m);
	assert(sha256_eq(&test_m, m));

	/* Two elements. */
	json_out("{\"comment\": \"n1 test, tlv1 = 1000, tlv2 = 1x2x3\",");
	json_out("\"tlv\": \"n1\",");
	all = concat(tlv1, tlv2);

	json_out("\"all-tlvs\": \"%s\",", tal_hex(tmpctx, all));
	json_out("\"leaves\": [");

	leaf[0] = H(LnBranch, ordered(H(LnLeaf, tlv1),
				      H(concat(LnAll, all), tlv1)));
	leaf[1] = H(LnBranch, ordered(H(LnLeaf, tlv2),
				      H(concat(LnAll, all), tlv2)));

	json_out("{ \"H(`LnLeaf`,%s)\": \"%s\", \"H(`LnAll`|all-tlvs,tlv1)\": \"%s\", \"H(`LnBranch`,leaf+nonce)\": \"%s\" },",
		 tal_hex(tmpctx, tlv1),
		 type_to_string(tmpctx, struct sha256, H(LnLeaf, tlv1)),
		 type_to_string(tmpctx, struct sha256, H(concat(LnAll, all), tlv1)),
		 type_to_string(tmpctx, struct sha256, leaf[0]));
	json_out("{ \"H(`LnLeaf`,%s)\": \"%s\", \"H(`LnAll`|all-tlvs,tlv2)\": \"%s\", \"H(`LnBranch`,leaf+nonce)\": \"%s\" }",
		 tal_hex(tmpctx, tlv2),
		 type_to_string(tmpctx, struct sha256, H(LnLeaf, tlv2)),
		 type_to_string(tmpctx, struct sha256, H(concat(LnAll, all), tlv2)),
		 type_to_string(tmpctx, struct sha256, leaf[1]));
	json_out("],");

	json_out("\"branches\": [");
	json_out("{ \"desc\": \"1: tlv1+nonce and tlv2+nonce\", \"H(`LnBranch`,%s)\": \"%s\" }",
		 tal_hex(tmpctx, ordered(leaf[0], leaf[1])),
		 tal_hex(tmpctx, H(LnBranch, ordered(leaf[0], leaf[1]))));
	json_out("],");

	m = H(LnBranch, ordered(leaf[0], leaf[1]));

	json_out("\"merkle\": \"%s\"",
		 type_to_string(tmpctx, struct sha256, m));
	json_out("},");

	printf("n1 = %s, merkle = %s\n",
	       tal_hex(tmpctx, all),
	       type_to_string(tmpctx, struct sha256, m));

	n1->tlv2 = tal(n1, struct short_channel_id);
	if (!mk_short_channel_id(n1->tlv2, 1, 2, 3))
		abort();
	merkle_n1(n1, &test_m);
	assert(sha256_eq(&test_m, m));

	/* Three elements. */
	json_out("{\"comment\": \"n1 test, tlv1 = 1000, tlv2 = 1x2x3, tlv3 = 0266e4598d1d3c415f572a8488830b60f7e744ed9235eb0b1ba93283b315c03518, 1, 2\",");
	json_out("\"tlv\": \"n1\",");
	all = concat(tlv1, tlv2, tlv3);
	json_out("\"all-tlvs\": \"%s\",", tal_hex(tmpctx, all));
	json_out("\"leaves\": [");

	leaf[0] = H(LnBranch, ordered(H(LnLeaf, tlv1),
				      H(concat(LnAll, all), tlv1)));
	leaf[1] = H(LnBranch, ordered(H(LnLeaf, tlv2),
				      H(concat(LnAll, all), tlv2)));
	leaf[2] = H(LnBranch, ordered(H(LnLeaf, tlv3),
				      H(concat(LnAll, all), tlv3)));
	json_out("{ \"H(`LnLeaf`,%s)\": \"%s\", \"H(`LnAll`|all-tlvs,tlv1)\": \"%s\", \"H(`LnBranch`,leaf+nonce)\": \"%s\" },",
		 tal_hex(tmpctx, tlv1),
		 type_to_string(tmpctx, struct sha256, H(LnLeaf, tlv1)),
		 type_to_string(tmpctx, struct sha256, H(concat(LnAll, all), tlv1)),
		 type_to_string(tmpctx, struct sha256, leaf[0]));
	json_out("{ \"H(`LnLeaf`,%s)\": \"%s\", \"H(`LnAll`|all-tlvs,tlv2)\": \"%s\", \"H(`LnBranch`,leaf+nonce)\": \"%s\" },",
		 tal_hex(tmpctx, tlv2),
		 type_to_string(tmpctx, struct sha256, H(LnLeaf, tlv2)),
		 type_to_string(tmpctx, struct sha256, H(concat(LnAll, all), tlv2)),
		 type_to_string(tmpctx, struct sha256, leaf[1]));
	json_out("{ \"H(`LnLeaf`,%s)\": \"%s\", \"H(`LnAll`|all-tlvs,tlv3)\": \"%s\", \"H(`LnBranch`,leaf+nonce)\": \"%s\" }",
		 tal_hex(tmpctx, tlv3),
		 type_to_string(tmpctx, struct sha256, H(LnLeaf, tlv3)),
		 type_to_string(tmpctx, struct sha256, H(concat(LnAll, all), tlv3)),
		 type_to_string(tmpctx, struct sha256, leaf[2]));
	json_out("],");

	json_out("\"branches\": [");
	json_out("{ \"desc\": \"1: tlv1+nonce and tlv2+nonce\", \"H(`LnBranch`,%s)\": \"%s\" },",
		 tal_hex(tmpctx, ordered(leaf[0], leaf[1])),
		 tal_hex(tmpctx, H(LnBranch, ordered(leaf[0], leaf[1]))));
	json_out("{ \"desc\": \"1 and tlv3+nonce\", \"H(`LnBranch`,%s)\": \"%s\" }",
		 tal_hex(tmpctx, ordered(H(LnBranch, ordered(leaf[0], leaf[1])),
					 leaf[2])),
		 tal_hex(tmpctx, H(LnBranch,
				   ordered(H(LnBranch,
					     ordered(leaf[0], leaf[1])),
					   leaf[2]))));
	json_out("],");

	m = H(LnBranch,
	      ordered(H(LnBranch, ordered(leaf[0], leaf[1])),
		      leaf[2]));
	json_out("\"merkle\": \"%s\"",
		 type_to_string(tmpctx, struct sha256, m));
	json_out("},");

	printf("n1 = %s, merkle = %s\n",
	       tal_hex(tmpctx, all),
	       type_to_string(tmpctx, struct sha256, m));

	n1->tlv3 = tal(n1, struct tlv_n1_tlv3);
	pubkey_from_hexstr("0266e4598d1d3c415f572a8488830b60f7e744ed9235eb0b1ba93283b315c03518", strlen("0266e4598d1d3c415f572a8488830b60f7e744ed9235eb0b1ba93283b315c03518"), &n1->tlv3->node_id);
	n1->tlv3->amount_msat_1 = AMOUNT_MSAT(1);
	n1->tlv3->amount_msat_2 = AMOUNT_MSAT(2);
	merkle_n1(n1, &test_m);
	assert(sha256_eq(&test_m, m));

	/* Now try with an actual offer, with 6 fields. */
	struct tlv_offer *offer = offer_decode(tmpctx,
					       "lno1qcp4256ypqpq86q2pucnq42ngssx2an9wfujqerp0y2pqun4wd68jtn00fkxzcnn9ehhyec6qgqsz83pqf9e58aguqr0rcun0ajlvmzq3ek63cw2w282gv3z5uupmuwvgjtq2",
					       strlen("lno1qcp4256ypqpq86q2pucnq42ngssx2an9wfujqerp0y2pqun4wd68jtn00fkxzcnn9ehhyec6qgqsz83pqf9e58aguqr0rcun0ajlvmzq3ek63cw2w282gv3z5uupmuwvgjtq2"),
					       NULL, NULL, &fail);

	assert(tal_count(offer->fields) == 6);
	u8 *fieldwires[6];

	/* currency: USD */
	fieldwires[0] = tlv(6, "USD", strlen("USD"));
	/* amount: 1000 */
	fieldwires[1] = tlv(8, "\x03\xe8", 2);
	/* description: 10USD every day */
	fieldwires[2] = tlv(10, "10USD every day", strlen("10USD every day"));
	/* issuer: rusty.ozlabs.org */
	fieldwires[3] = tlv(20, "rusty.ozlabs.org", strlen("rusty.ozlabs.org"));
	/* recurrence: time_unit = 1, period = 1 */
	fieldwires[4] = tlv(26, "\x01\x01", 2);
	/* node_id: 024b9a1fa8e006f1e3937f65f66c408e6da8e1ca728ea43222a7381df1cc449605 */
	fieldwires[5] = tlv(30, "\x02\x4b\x9a\x1f\xa8\xe0\x06\xf1\xe3\x93\x7f\x65\xf6\x6c\x40\x8e\x6d\xa8\xe1\xca\x72\x8e\xa4\x32\x22\xa7\x38\x1d\xf1\xcc\x44\x96\x05", 33);

	json_out("{\"comment\": \"offer test, currency = USD, amount = 1000, description = 10USD every day, issuer = rusty.ozlabs.org, recurrence = time_unit = 1, period = 1, node_id = 024b9a1fa8e006f1e3937f65f66c408e6da8e1ca728ea43222a7381df1cc449605\",");
	json_out("\"tlv\": \"offer\",");

	all = concat(fieldwires[0], fieldwires[1], fieldwires[2],
		     fieldwires[3], fieldwires[4], fieldwires[5]);
	json_out("\"all-tlvs\": \"%s\",", tal_hex(tmpctx, all));

	json_out("\"leaves\": [");
	for (size_t i = 0; i < ARRAY_SIZE(fieldwires); i++) {
		leaf[i] = H(LnBranch,
			    ordered(H(LnLeaf, fieldwires[i]),
				    H(concat(LnAll, all), fieldwires[i])));
		json_out("{ \"H(`LnLeaf`,%s)\": \"%s\", \"H(`LnAll`|all-tlvs,tlv)\": \"%s\", \"H(`LnBranch`,leaf+nonce)\": \"%s\" }%s",
			 tal_hex(tmpctx, fieldwires[i]),
			 type_to_string(tmpctx, struct sha256,
					H(LnLeaf, fieldwires[i])),
			 type_to_string(tmpctx, struct sha256,
					H(concat(LnAll, all), fieldwires[0])),
			 type_to_string(tmpctx, struct sha256, leaf[i]),
			 i == ARRAY_SIZE(fieldwires) - 1 ? "" : ",");
	}
	json_out("],");

	json_out("\"branches\": [");
	json_out("{ \"desc\": \"1: currency+nonce and amount+nonce\", \"H(`LnBranch`,%s)\": \"%s\" },",
		 tal_hex(tmpctx, ordered(leaf[0], leaf[1])),
		 tal_hex(tmpctx, H(LnBranch, ordered(leaf[0], leaf[1]))));
	json_out("{ \"desc\": \"2: description+nonce and issuer+nonce\", \"H(`LnBranch`,%s)\": \"%s\"},",
		 tal_hex(tmpctx, ordered(leaf[2], leaf[3])),
		 tal_hex(tmpctx, H(LnBranch, ordered(leaf[2], leaf[3]))));
	struct sha256 *b12 = H(LnBranch,
			       ordered(H(LnBranch,
					 ordered(leaf[0], leaf[1])),
				       H(LnBranch,
					 ordered(leaf[2], leaf[3]))));
	json_out("{ \"desc\": \"3: 1 and 2\", \"H(`LnBranch`,%s)\": \"%s\" },",
		 tal_hex(tmpctx, ordered(H(LnBranch, ordered(leaf[0], leaf[1])),
					 H(LnBranch, ordered(leaf[2], leaf[3])))),
		 tal_hex(tmpctx, b12));
	json_out("{ \"desc\": \"4: recurrence+nonce and node_id+nonce\", \"H(`LnBranch`,%s)\": \"%s\" },",
		 tal_hex(tmpctx, ordered(leaf[4], leaf[5])),
		 tal_hex(tmpctx, H(LnBranch, ordered(leaf[4], leaf[5]))));
	json_out("{ \"desc\": \"5: 3 and 4\", \"H(`LnBranch`,%s)\": \"%s\" }",
		 tal_hex(tmpctx, ordered(b12,
					 H(LnBranch,
					   ordered(leaf[4], leaf[5])))),
		 tal_hex(tmpctx, H(LnBranch,
				   ordered(b12,
					   H(LnBranch,
					     ordered(leaf[4], leaf[5]))))));
	m = H(LnBranch,
	      ordered(H(LnBranch,
			ordered(H(LnBranch, ordered(leaf[0], leaf[1])),
				H(LnBranch, ordered(leaf[2], leaf[3])))),
		      H(LnBranch, ordered(leaf[4], leaf[5]))));

	json_out("],");
	json_out("\"merkle\": \"%s\"",
		 type_to_string(tmpctx, struct sha256, m));
	json_out("}]");

	printf("offer = %s, merkle = %s\n",
	       tal_hex(tmpctx, all),
	       type_to_string(tmpctx, struct sha256, m));

	merkle_tlv(offer->fields, &test_m);
	assert(sha256_eq(&test_m, m));

	common_shutdown();
}

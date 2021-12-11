#include "config.h"
#include <bitcoin/privkey.h>
#include <bitcoin/pubkey.h>
#include <common/key_derive.h>
#include <common/utils.h>
#include <wally_bip32.h>

/* BOLT #3:
 *
 * ### `localpubkey`, `local_htlcpubkey`, `remote_htlcpubkey`, `local_delayedpubkey`, and `remote_delayedpubkey` Derivation
 *
 * These pubkeys are simply generated by addition from their base points:
 *
 *	pubkey = basepoint + SHA256(per_commitment_point || basepoint) * G
 *
 * The `localpubkey` uses the local node's `payment_basepoint`;
 * The `remotepubkey` uses the remote node's `payment_basepoint`;
 * the `local_htlcpubkey` uses the local node's `htlc_basepoint`;
 * the `remote_htlcpubkey` uses the remote node's `htlc_basepoint`;
 * the `local_delayedpubkey` uses the local node's `delayed_payment_basepoint`;
 * and the `remote_delayedpubkey` uses the remote node's `delayed_payment_basepoint`.
 *...
 * If `option_static_remotekey` or `option_anchors` is negotiated, the
 * `remotepubkey` is simply the remote node's `payment_basepoint`, otherwise
 * it is calculated as above using the remote node's `payment_basepoint`.
 */
bool derive_simple_key(const struct pubkey *basepoint,
		    const struct pubkey *per_commitment_point,
		    struct pubkey *key)
{
	struct sha256 sha;
	unsigned char der_keys[PUBKEY_CMPR_LEN * 2];

	pubkey_to_der(der_keys, per_commitment_point);
	pubkey_to_der(der_keys + PUBKEY_CMPR_LEN, basepoint);
	sha256(&sha, der_keys, sizeof(der_keys));
#ifdef SUPERVERBOSE
	printf("# SHA256(per_commitment_point || basepoint)\n");
	printf("# => SHA256(0x%s || 0x%s)\n",
	       tal_hexstr(tmpctx, der_keys, PUBKEY_CMPR_LEN),
	       tal_hexstr(tmpctx, der_keys + PUBKEY_CMPR_LEN, PUBKEY_CMPR_LEN));
	printf("# = 0x%s\n",
	       tal_hexstr(tmpctx, &sha, sizeof(sha)));
#endif

	*key = *basepoint;
	if (secp256k1_ec_pubkey_tweak_add(secp256k1_ctx,
					  &key->pubkey, sha.u.u8) != 1)
		return false;
#ifdef SUPERVERBOSE
	printf("# + basepoint (0x%s)\n",
	       type_to_string(tmpctx, struct pubkey, basepoint));
	printf("# = 0x%s\n",
	       type_to_string(tmpctx, struct pubkey, key));
#endif
	return true;
}

/* BOLT #3:
 *
 * The corresponding private keys can be similarly derived, if the basepoint
 * secrets are known (i.e. the private keys corresponding to `localpubkey`,
 * `local_htlcpubkey`, and `local_delayedpubkey` only):
 *
 *     privkey = basepoint_secret + SHA256(per_commitment_point || basepoint)
 */
bool derive_simple_privkey(const struct secret *base_secret,
			const struct pubkey *basepoint,
			const struct pubkey *per_commitment_point,
			struct privkey *key)
{
	struct sha256 sha;
	unsigned char der_keys[PUBKEY_CMPR_LEN * 2];

	pubkey_to_der(der_keys, per_commitment_point);
	pubkey_to_der(der_keys + PUBKEY_CMPR_LEN, basepoint);
	sha256(&sha, der_keys, sizeof(der_keys));
#ifdef SUPERVERBOSE
	printf("# SHA256(per_commitment_point || basepoint)\n");
	printf("# => SHA256(0x%s || 0x%s)\n",
	       tal_hexstr(tmpctx, der_keys, PUBKEY_CMPR_LEN),
	       tal_hexstr(tmpctx, der_keys + PUBKEY_CMPR_LEN, PUBKEY_CMPR_LEN));
	printf("# = 0x%s\n", tal_hexstr(tmpctx, &sha, sizeof(sha)));
#endif

	key->secret = *base_secret;
	if (secp256k1_ec_privkey_tweak_add(secp256k1_ctx, key->secret.data,
					   sha.u.u8) != 1)
		return false;
#ifdef SUPERVERBOSE
	printf("# + basepoint_secret (0x%s)\n",
	       tal_hexstr(tmpctx, base_secret, sizeof(*base_secret)));
	printf("# = 0x%s\n",
	       tal_hexstr(tmpctx, key, sizeof(*key)));
#endif
	return true;
}

/* BOLT #3:
 *
 * The `revocationpubkey` is a blinded key: when the local node wishes to
 * create a new commitment for the remote node, it uses its own
 * `revocation_basepoint` and the remote node's `per_commitment_point` to
 * derive a new `revocationpubkey` for the commitment. After the remote node
 * reveals the `per_commitment_secret` used (thereby revoking that
 * commitment), the local node can then derive the `revocationprivkey`, as it
 * now knows the two secrets necessary to derive the key
 * (`revocation_basepoint_secret` and `per_commitment_secret`).
 *
 * The `per_commitment_point` is generated using elliptic-curve multiplication:
 *
 * 	per_commitment_point = per_commitment_secret * G
 *
 * And this is used to derive the revocation pubkey from the remote node's
 * `revocation_basepoint`:
 *
 * 	revocationpubkey = revocation_basepoint * SHA256(revocation_basepoint || per_commitment_point) + per_commitment_point * SHA256(per_commitment_point || revocation_basepoint)
 */
bool derive_revocation_key(const struct pubkey *basepoint,
			const struct pubkey *per_commitment_point,
			struct pubkey *key)
{
	struct sha256 sha;
	unsigned char der_keys[PUBKEY_CMPR_LEN * 2];
	secp256k1_pubkey add[2];
	const secp256k1_pubkey *args[2];

	pubkey_to_der(der_keys, basepoint);
	pubkey_to_der(der_keys + PUBKEY_CMPR_LEN, per_commitment_point);
	sha256(&sha, der_keys, sizeof(der_keys));
#ifdef SUPERVERBOSE
	printf("# SHA256(revocation_basepoint || per_commitment_point)\n");
	printf("# => SHA256(0x%s || 0x%s)\n",
	       tal_hexstr(tmpctx, der_keys, PUBKEY_CMPR_LEN),
	       tal_hexstr(tmpctx, der_keys + PUBKEY_CMPR_LEN, PUBKEY_CMPR_LEN));
	printf("# = 0x%s\n", tal_hexstr(tmpctx, sha.u.u8, sizeof(sha.u.u8))),
#endif

	add[0] = basepoint->pubkey;
	if (secp256k1_ec_pubkey_tweak_mul(secp256k1_ctx, &add[0], sha.u.u8) != 1)
		return false;
#ifdef SUPERVERBOSE
	printf("# x revocation_basepoint = 0x%s\n",
	       type_to_string(tmpctx, secp256k1_pubkey, &add[0]));
#endif

	pubkey_to_der(der_keys, per_commitment_point);
	pubkey_to_der(der_keys + PUBKEY_CMPR_LEN, basepoint);
	sha256(&sha, der_keys, sizeof(der_keys));
#ifdef SUPERVERBOSE
	printf("# SHA256(per_commitment_point || revocation_basepoint)\n");
	printf("# => SHA256(0x%s || 0x%s)\n",
	       tal_hexstr(tmpctx, der_keys, PUBKEY_CMPR_LEN),
	       tal_hexstr(tmpctx, der_keys + PUBKEY_CMPR_LEN, PUBKEY_CMPR_LEN));
	printf("# = 0x%s\n", tal_hexstr(tmpctx, sha.u.u8, sizeof(sha.u.u8))),
#endif

	add[1] = per_commitment_point->pubkey;
	if (secp256k1_ec_pubkey_tweak_mul(secp256k1_ctx, &add[1], sha.u.u8) != 1)
		return false;
#ifdef SUPERVERBOSE
	printf("# x per_commitment_point = 0x%s\n",
	       type_to_string(tmpctx, secp256k1_pubkey, &add[1]));
#endif

	args[0] = &add[0];
	args[1] = &add[1];
	if (secp256k1_ec_pubkey_combine(secp256k1_ctx, &key->pubkey, args, 2)
	    != 1)
		return false;

#ifdef SUPERVERBOSE
	printf("# 0x%s + 0x%s => 0x%s\n",
	       type_to_string(tmpctx, secp256k1_pubkey, args[0]),
	       type_to_string(tmpctx, secp256k1_pubkey, args[1]),
	       type_to_string(tmpctx, struct pubkey, key));
#endif
	return true;
}

/* BOLT #3:
 *
 * The corresponding private key can be derived once the `per_commitment_secret`
 * is known:
 *
 *     revocationprivkey = revocation_basepoint_secret * SHA256(revocation_basepoint || per_commitment_point) + per_commitment_secret * SHA256(per_commitment_point || revocation_basepoint)
 */
bool derive_revocation_privkey(const struct secret *base_secret,
			       const struct secret *per_commitment_secret,
			       const struct pubkey *basepoint,
			       const struct pubkey *per_commitment_point,
			       struct privkey *key)
{
	struct sha256 sha;
	unsigned char der_keys[PUBKEY_CMPR_LEN * 2];
	struct secret part2;

	pubkey_to_der(der_keys, basepoint);
	pubkey_to_der(der_keys + PUBKEY_CMPR_LEN, per_commitment_point);
	sha256(&sha, der_keys, sizeof(der_keys));
#ifdef SUPERVERBOSE
	printf("# SHA256(revocation_basepoint || per_commitment_point)\n");
	printf("# => SHA256(0x%s || 0x%s)\n",
	       tal_hexstr(tmpctx, der_keys, PUBKEY_CMPR_LEN),
	       tal_hexstr(tmpctx, der_keys + PUBKEY_CMPR_LEN, PUBKEY_CMPR_LEN));
	printf("# = 0x%s\n", tal_hexstr(tmpctx, sha.u.u8, sizeof(sha.u.u8))),
#endif

	key->secret = *base_secret;
	if (secp256k1_ec_privkey_tweak_mul(secp256k1_ctx, key->secret.data,
					   sha.u.u8)
	    != 1)
		return false;
#ifdef SUPERVERBOSE
	printf("# * revocation_basepoint_secret (0x%s)",
	       tal_hexstr(tmpctx, base_secret, sizeof(*base_secret))),
	printf("# = 0x%s\n", tal_hexstr(tmpctx, key, sizeof(*key))),
#endif

	pubkey_to_der(der_keys, per_commitment_point);
	pubkey_to_der(der_keys + PUBKEY_CMPR_LEN, basepoint);
	sha256(&sha, der_keys, sizeof(der_keys));
#ifdef SUPERVERBOSE
	printf("# SHA256(per_commitment_point || revocation_basepoint)\n");
	printf("# => SHA256(0x%s || 0x%s)\n",
	       tal_hexstr(tmpctx, der_keys, PUBKEY_CMPR_LEN),
	       tal_hexstr(tmpctx, der_keys + PUBKEY_CMPR_LEN, PUBKEY_CMPR_LEN));
	printf("# = 0x%s\n", tal_hexstr(tmpctx, sha.u.u8, sizeof(sha.u.u8))),
#endif

	part2 = *per_commitment_secret;
	if (secp256k1_ec_privkey_tweak_mul(secp256k1_ctx, part2.data,
					   sha.u.u8) != 1)
		return false;
#ifdef SUPERVERBOSE
	printf("# * per_commitment_secret (0x%s)",
	       tal_hexstr(tmpctx, per_commitment_secret,
			  sizeof(*per_commitment_secret))),
		printf("# = 0x%s\n", tal_hexstr(tmpctx, &part2, sizeof(part2)));
#endif

	if (secp256k1_ec_privkey_tweak_add(secp256k1_ctx, key->secret.data,
					   part2.data) != 1)
		return false;

#ifdef SUPERVERBOSE
	printf("# => 0x%s\n", tal_hexstr(tmpctx, key, sizeof(*key)));
#endif
	return true;
}


bool bip32_pubkey(const struct ext_key *bip32_base,
		  struct pubkey *pubkey, u32 index)
{
	const uint32_t flags = BIP32_FLAG_KEY_PUBLIC | BIP32_FLAG_SKIP_HASH;
	struct ext_key ext;

	if (index >= BIP32_INITIAL_HARDENED_CHILD)
		return false;

	if (bip32_key_from_parent(bip32_base, index, flags, &ext) != WALLY_OK)
		return false;

	if (!secp256k1_ec_pubkey_parse(secp256k1_ctx, &pubkey->pubkey,
				       ext.pub_key, sizeof(ext.pub_key)))
		return false;
	return true;
}

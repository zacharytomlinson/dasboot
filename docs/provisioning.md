# Provisioning (ATECC608A + Firmware Signing Keys)

## Key Format Sanity Check

When using the raw 64-byte formats expected by the ATECC verify flow:

- Public key: `X || Y` where `X` and `Y` are 32-byte **big-endian** integers (P-256 field elements).
- Signature: `R || S` where `R` and `S` are 32-byte **big-endian** integers.

Note: many software stacks represent ECDSA signatures as **ASN.1 DER** (`SEQUENCE(INTEGER r, INTEGER s)`), which must be converted to raw `R||S` for provisioning/on-wire use.

## Generate P-256 Keypair (Debian)

Generate a signing keypair on `prime256v1` (secp256r1 / NIST P-256):

```bash
openssl genpkey -algorithm EC -pkeyopt ec_paramgen_curve:prime256v1 -out fw_priv.pem
openssl pkey -in fw_priv.pem -pubout -out fw_pub.pem
```

## Extract Raw Public Key (`X||Y`) for Provisioning

Extract the raw 64-byte public key (`X||Y`, big-endian) from `fw_pub.pem`:

```bash
python3 - <<'PY'
from cryptography.hazmat.primitives import serialization
pub = serialization.load_pem_public_key(open("fw_pub.pem","rb").read())
n = pub.public_numbers()
open("fw_pub_xy.bin","wb").write(n.x.to_bytes(32,"big")+n.y.to_bytes(32,"big"))
print("wrote fw_pub_xy.bin (64 bytes)")
PY
```

This `fw_pub_xy.bin` is the 64-byte value intended to be stored in the ATECC slot used for firmware verification (planned slot: **8**).

## Generate a Raw Signature (`R||S`)

Many tools output DER signatures; convert DER → raw `R||S` as 64 bytes:

```bash
python3 - <<'PY'
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import ec, utils
priv = serialization.load_pem_private_key(open("fw_priv.pem","rb").read(), password=None)
msg = open("message.bin","rb").read()  # message bytes to sign (see note below)
sig_der = priv.sign(msg, ec.ECDSA(hashes.SHA256()))
r,s = utils.decode_dss_signature(sig_der)
open("sig_rs.bin","wb").write(r.to_bytes(32,"big")+s.to_bytes(32,"big"))
print("wrote sig_rs.bin (64 bytes)")
PY
```

If `message.bin` is *already* a 32-byte SHA-256 hash that should be signed “as-is”, use `utils.Prehashed(hashes.SHA256())` instead of `hashes.SHA256()` in the signing call.

## Device Provisioning Notes

- Slot 8 is planned for the firmware verification public key.
- Provisioning should lock the slot after writing the public key (locking is permanent).

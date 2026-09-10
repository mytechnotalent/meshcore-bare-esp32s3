idf_component_register(SRCS "main.c" "ble_companion.c" "runtime.c" "companion.c" "mesh_packet.c" "node_state.c" "storage.c" "identity.c" "advert.c" "mesh_crypto.c" "mesh_transport.c" "datagram.c"
                    "sx1262.c" "../crypto/ed25519/add_scalar.c" "../crypto/ed25519/fe.c"
                    "../crypto/ed25519/ge.c" "../crypto/ed25519/key_exchange.c"
                    "../crypto/ed25519/keypair.c" "../crypto/ed25519/sc.c"
                    "../crypto/ed25519/seed.c" "../crypto/ed25519/sha512.c"
                    "../crypto/ed25519/sign.c" "../crypto/ed25519/verify.c"
                    INCLUDE_DIRS "." "../crypto/ed25519")
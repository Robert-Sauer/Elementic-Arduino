# Metadata ABI 1 manual adapter

Copy `ElementicBuildMetadata.cpp` into `src/`, select the exact target/profile values and keep firmware Project ID zero unless the Website has allocated a full 64-bit identity. Do not widen or reinterpret legacy `projectid`.

Generated projects provide the same external symbols plus `elementicLoadFactoryDefaults()`. The compiled loader must clear runtime registries, apply network/MQTT/input/output defaults, retain unit provisioning and be safe when called repeatedly. Password literals intentionally remain in authorized generated source so offline factory reset can restore them.

Wire selection is independent: the default is numeric v1. Add `-DELEMENTIC_WIRE_PROTOCOL_VERSION=2` only after the compatible Android release is available to the intended users.

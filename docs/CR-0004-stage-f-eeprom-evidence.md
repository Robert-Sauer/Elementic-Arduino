# CR-0004 Stage F EEPROM and wire evidence

This record distinguishes source-derived compatibility evidence from physical captures. No physical EEPROM was read during Stage F.

## Released source result

The only Git tag is `v1.0.0` at `42e6f53`; the Stage F base is `3f9999fd6103250482eba6a2fb436c1dd3a2c53f` (`library.json` 1.0.1). Both revisions contain the same serialization behavior:

- full synchronization starts with `S01`, ASCII type `0x31`, ASCII version `0x31`;
- EEPROM starts with numeric type `0x01`, numeric schema version `0x01`;
- a schema-1 system record consists of the four-byte header `category, field, id=0, length=0` followed by a two-byte little-endian payload;
- the committed reader consumes those two payload bytes and therefore stays aligned with the committed writer.

The old reader comment saying that a system record was only four bytes is wrong. The pre-existing uncommitted edit found at Stage F start changed the reader to interpret header `id/length` as the value. That edit would return zero for the writer's count records and leave the actual two payload bytes to be parsed as another header. It is not a released layout and was superseded by the fixture-backed six-byte behavior.

Fixtures are in `test/fixtures/released-layouts.json`. They cover the tagged ASCII wire header, headerless updates, directional integer byte order, six-byte system records, following-record alignment and rejection of the speculative four-byte shape.

## Migration decision

Stage F accepts a schema-1 image only when a complete bounded validation pass recognizes the released layout. Loading it does not change its bytes. The next explicit Save writes storage schema 2; merely receiving numeric wire version `0x01` does not rewrite EEPROM.

Unknown, truncated, four-byte or otherwise ambiguous schema-1 data receives `MIGRATION_REQUIRED` and is not applied or repaired. Schema 2 uses two CRC/commit slots. Per-unit MAC provisioning uses separate `EUP1` slots outside both configuration slots and is retained by factory reset.

Installed-device captures remain a release gate. Add their immutable hashes and board/library provenance here before authorizing any additional schema-1 migration shape.

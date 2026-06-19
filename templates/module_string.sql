-- TEMPLATE — add rows to data/sql/db-world/base/sod_mage_module_string.sql.
-- Not applied from templates/. Idempotent: the file re-DELETEs all its own rows
-- (WHERE module = @MODULE_STRING) then re-INSERTs. Model: that file.
--
-- Player-facing text goes through module_string, never hardcoded literals. Read it
-- in C++ with PSendModuleSysMessage(MODULE_STRING, <id>, ...). Color codes work,
-- e.g. |cff4CFF00green|r.

SET @MODULE_STRING := 'mod-sod-mage';

DELETE FROM `module_string` WHERE `module` = @MODULE_STRING;
INSERT INTO `module_string` (`module`, `id`, `string`) VALUES
(@MODULE_STRING, 1, 'This server is running the |cff4CFF00SoD Mage|r module.'),
(@MODULE_STRING, <ID>, '<Your message. Use $tokens or |cRRGGBB...|r as needed.>');

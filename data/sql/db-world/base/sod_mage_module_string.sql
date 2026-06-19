SET @MODULE_STRING := 'mod-sod-mage';

-- module string. Upsert (REPLACE) rather than DELETE+INSERT -- committed SQL must
-- never DELETE (it runs on every end-user DB); see the module CLAUDE.md SQL rules.
REPLACE INTO `module_string` (`module`, `id`, `string`) VALUES
(@MODULE_STRING, 1, 'This server is running the |cff4CFF00SoD Mage|r module.');

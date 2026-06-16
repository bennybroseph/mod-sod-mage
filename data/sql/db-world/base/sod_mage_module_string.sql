SET @MODULE_STRING := 'mod-sod-mage';

-- module string
DELETE FROM `module_string` WHERE `module` = @MODULE_STRING;
INSERT INTO `module_string` (`module`, `id`, `string`) VALUES
(@MODULE_STRING, 1, 'This server is running the |cff4CFF00SoD Mage|r module.');

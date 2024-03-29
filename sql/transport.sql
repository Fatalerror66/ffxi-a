/*
MySQL Data Transfer
Source Host: localhost
Source Database: database
Target Host: localhost
Target Database: database
Date: 9/14/2013 11:03:54 PM
*/

SET FOREIGN_KEY_CHECKS=0;
-- ----------------------------
-- Table structure for transport
-- ----------------------------
CREATE TABLE `transport` (
  `id` tinyint(3) unsigned NOT NULL,
  `name` tinytext NOT NULL,
  `transport` int(10) unsigned NOT NULL DEFAULT '0',
  `door` int(10) unsigned NOT NULL DEFAULT '0',
  `dock_x` float(7,3) NOT NULL DEFAULT '0.000',
  `dock_y` float(7,3) NOT NULL DEFAULT '0.000',
  `dock_z` float(7,3) NOT NULL DEFAULT '0.000',
  `dock_rot` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `boundary` smallint(5) unsigned NOT NULL DEFAULT '0',
  `anim_arrive` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `anim_depart` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `time_offset` smallint(5) unsigned NOT NULL DEFAULT '0',
  `time_interval` smallint(5) unsigned NOT NULL DEFAULT '0',
  `time_anim_arrive` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `time_waiting` smallint(5) unsigned NOT NULL DEFAULT '0',
  `time_anim_depart` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `zone` tinyint(3) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records 
-- ----------------------------
INSERT INTO `transport` VALUES ('1', 'Kazham-Jeuno_Airship', '17801316', '17801315', '-4.000', '-3.497', '32.125', '128', '512', '18', '19', '90', '360', '20', '50', '20', '226');
INSERT INTO `transport` VALUES ('2', 'Jeuno-Kazham_Airship', '17784932', '17784931', '-4.000', '10.450', '117.000', '64', '477', '24', '25', '270', '360', '20', '50', '20', '226');
INSERT INTO `transport` VALUES ('3', 'Jeuno-Sandoria_Airship', '17784932', '17784928', '-68.000', '10.450', '117.000', '64', '474', '18', '19', '0', '360', '12', '60', '12', '223');
INSERT INTO `transport` VALUES ('4', 'Jeuno-Windurst_Airship', '17784932', '17784930', '-6.000', '10.450', '-117.000', '192', '476', '22', '23', '90', '360', '12', '60', '12', '225');
INSERT INTO `transport` VALUES ('5', 'Jeuno-Bastok_Airship', '17784932', '17784929', '-70.000', '10.450', '-117.000', '192', '475', '20', '21', '180', '360', '12', '60', '16', '224');
INSERT INTO `transport` VALUES ('6', 'Sandoria-Jeuno_Airship', '17727594', '17727591', '20.000', '-2.000', '44.000', '64', '369', '18', '19', '180', '360', '12', '60', '16', '223');
INSERT INTO `transport` VALUES ('7', 'Windurst-Jeuno_Airship', '17760416', '17760415', '242.281', '-3.522', '61.994', '96', '416', '18', '19', '270', '360', '18', '60', '14', '225');
INSERT INTO `transport` VALUES ('8', 'Bastok-Jeuno_Airship', '17743967', '17743961', '-36.458', '6.365', '-77.322', '128', '315', '18', '19', '0', '360', '14', '60', '16', '224');
INSERT INTO `transport` VALUES ('9', 'Selbina-Mhaura_Boat', '17793084', '17793083', '9.294', '0.000', '-69.775', '0', '485', '18', '19', '372', '480', '18', '90', '17', '220');
INSERT INTO `transport` VALUES ('10', 'Mhaura-Selbina_Boat', '17797178', '17797177', '-0.516', '0.003', '-8.409', '0', '493', '18', '19', '372', '480', '18', '90', '17', '221');
INSERT INTO `transport` VALUES ('11', 'Mhaura-Whitegate_Boat', '17797178', '17797177', '-0.516', '0.003', '-8.409', '0', '493', '18', '19', '142', '480', '18', '80', '17', '46');
INSERT INTO `transport` VALUES ('12', 'Whitegate-Mhaura_Boat', '16982041', '16982039', '-20.726', '3.389', '-154.231', '0', '569', '18', '19', '142', '480', '18', '80', '16', '47');
INSERT INTO `transport` VALUES ('13', 'Whitegate-Nashmau_Boat', '16982041', '16982040', '20.726', '3.389', '154.231', '128', '570', '20', '21', '282', '480', '18', '180', '17', '58');
INSERT INTO `transport` VALUES ('14', 'Nashmau-Whitegate_Boat', '16994322', '16994321', '3.240', '3.389', '-114.221', '0', '571', '22', '23', '282', '480', '18', '180', '16', '59');
INSERT INTO `transport` VALUES ('15', 'Manaclip_PUR_BIBI', '16793907', '16793908', '491.500', '0.000', '687.400', '128', '0', '18', '19', '0', '434', '20', '80', '20', '3');
INSERT INTO `transport` VALUES ('16', 'MANA_BIBI_PUR', '16793907', '16793909', '-392.000', '0.000', '-364.000', '128', '0', '20', '21', '300', '434', '20', '80', '20', '3');

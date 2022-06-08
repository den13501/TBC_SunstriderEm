DROP PROCEDURE IF EXISTS DeleteCreature;
delimiter ??
CREATE PROCEDURE DeleteCreature (param1 INT)
BEGIN
    DELETE FROM creature_entry WHERE spawnID = param1; 
    DELETE FROM creature_addon WHERE spawnID = param1; 
    DELETE c1, c2, sg FROM creature c 
    LEFT JOIN conditions c1 ON c1.ConditionTypeOrReference = 31 AND c1.ConditionValue1 = 3 AND c1.ConditionValue3 = c.spawnID 
    LEFT JOIN conditions c2 ON c2.SourceTypeOrReferenceId = 22 AND c2.SourceId = 0 AND c1.SourceEntry = -c.spawnID 
    LEFT JOIN spawn_group sg ON sg.spawnId = c.spawnID AND spawnType = 0 
    WHERE c.spawnID = param1;
    -- incomplete here...
    DELETE FROM creature_formations WHERE memberGUID = param1;
    DELETE FROM creature_movement_override WHERE SpawnId = param1;
    DELETE FROM creature WHERE spawnID = param1;
END??

delimiter ;

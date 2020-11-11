ALTER TABLE `creature`
ADD COLUMN `entry`  mediumint(8) NULL AFTER `spawnID`;

UPDATE
creature a,
creature_entry b
SET
a.entry=b.entry
WHERE
a.spawnID=b.spawnID;

ALTER TABLE `creature`
MODIFY COLUMN `entry`  mediumint(8) NOT NULL AFTER `spawnID`;
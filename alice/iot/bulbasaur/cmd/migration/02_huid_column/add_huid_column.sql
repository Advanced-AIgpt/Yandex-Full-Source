ALTER TABLE DeviceGroups ADD COLUMN huid uint64;
ALTER TABLE DeviceGroups ADD COLUMN user_id uint64;

ALTER TABLE Devices ADD COLUMN huid uint64;
ALTER TABLE Devices ADD COLUMN user_id uint64;

ALTER TABLE Groups ADD COLUMN huid uint64;

ALTER TABLE Rooms ADD COLUMN huid uint64;

ALTER TABLE Scenarios ADD COLUMN huid uint64;
ALTER TABLE Scenarios ADD COLUMN user_id uint64;

ALTER TABLE Users ADD COLUMN hid uint64;

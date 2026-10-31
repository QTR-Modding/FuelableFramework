#include "Manager.h"

void BaseFormFloat::DumpToLog() {
    Locker locker(m_Lock);
    for (const auto& [formId, value] : m_Data) {
        logger::info("Dump Row From {} - FormID1st: {} - FormID2nd: {} - value: {}", GetType(), formId.outerKey,
                     formId.innerKey, value);
    }
    // sakat olabilir
    logger::info("{} Rows Dumped For Type {}", m_Data.size(), GetType());
};
void SaveCallback(SKSE::SerializationInterface* serializationInterface) {
    auto* LSM = LightSourceManager::GetSingleton();
    if (!LSM->current_source) logger::info("Saving...btw, current_source is nullptr.");
    if (LSM->is_burning) LSM->PauseBurn();
    LSM->SendData();
    LSM->LogRemainings();
    if (!LSM->Save(serializationInterface, kDataKey, kSerializationVersion)) {
        logger::critical("Failed to save Data");
    }
    uint32_t equipped_obj_id = LSM->current_source ? LSM->current_source->formid : 0;
    uint32_t fuel_id = LSM->current_source ? LSM->current_source->fuel : 0;
    serializationInterface->WriteRecordData(equipped_obj_id);
    serializationInterface->WriteRecordData(fuel_id);
    // QL data
    serializationInterface->WriteRecordData(Hooks::ql_elapsed_t);
    serializationInterface->WriteRecordData(Hooks::ql_depleted);
    logger::trace("QL data saved. {} {}", Hooks::ql_elapsed_t, Hooks::ql_depleted);

    if (LSM->is_burning) {
        LSM->UnPauseBurn();
        logger::info("Data Saved");
    }
}

void LoadCallback(SKSE::SerializationInterface* serializationInterface) {
    auto* LSM = LightSourceManager::GetSingleton();
    std::uint32_t type;
    std::uint32_t version;
    std::uint32_t length;
    uint32_t equipped_obj_id;
    uint32_t fuel_id;

    while (serializationInterface->GetNextRecordInfo(type, version, length)) {
        auto temp = Utilities::DecodeTypeCode(type);

        if (version != kSerializationVersion-1 && version != kSerializationVersion) {
            logger::critical("Loaded data has incorrect version. Recieved ({}) - Expected ({}) for Data Key ({})",
                             version, kSerializationVersion, temp);
            continue;
        }
        switch (type) {
            case kDataKey: {
                if (!LSM->Load(serializationInterface)) {
                    logger::critical("Failed to Load Data");
                }
                serializationInterface->ReadRecordData(equipped_obj_id);
                serializationInterface->ReadRecordData(fuel_id);
                if (equipped_obj_id) {
                    if (!serializationInterface->ResolveFormID(equipped_obj_id, equipped_obj_id) ||
                        !serializationInterface->ResolveFormID(fuel_id, fuel_id) ||
                        !LSM->SetSource(equipped_obj_id, fuel_id)) {
                        MsgBoxesNotifs::InGame::LoadOrderError();
                    }
                }
            } break;
            default:
                logger::critical("Unrecognized Record Type: {}", temp);
                break;
        }
        if (version == kSerializationVersion) {
            serializationInterface->ReadRecordData(Hooks::ql_elapsed_t);
            serializationInterface->ReadRecordData(Hooks::ql_depleted);
            logger::trace("QL data loaded. {} {}", Hooks::ql_elapsed_t, Hooks::ql_depleted);
        } else {
            Hooks::ql_depleted = true;
        }
    }
    LSM->ReceiveData();
    logger::info("Data loaded from skse co-save.");
}

void InitializeSerialization() {
    auto* serialization = SKSE::GetSerializationInterface();
    serialization->SetUniqueID(kDataKey);
    serialization->SetSaveCallback(SaveCallback);
    serialization->SetLoadCallback(LoadCallback);
    logger::info("Cosave serialization initialized.");
}

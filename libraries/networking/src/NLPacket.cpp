//
//  NLPacket.cpp
//  libraries/networking/src
//
//  Created by Clement on 7/6/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "NLPacket.h"

qint64 NLPacket::localHeaderSize(PacketType::Value type) {
    qint64 size = ((NON_SOURCED_PACKETS.contains(type)) ? 0 : NUM_BYTES_RFC4122_UUID) +
                    ((NON_VERIFIED_PACKETS.contains(type)) ? 0 : NUM_BYTES_RFC4122_UUID);
    return size;
}

qint64 NLPacket::maxPayloadSize(PacketType::Value type) {
    return Packet::maxPayloadSize(type) - localHeaderSize(type);
}

qint64 NLPacket::totalHeadersSize() const {
    return localHeaderSize() + Packet::localHeaderSize();
}

qint64 NLPacket::localHeaderSize() const {
    return localHeaderSize(_type);
}

std::unique_ptr<NLPacket> NLPacket::create(PacketType::Value type, qint64 size) {
    auto maxPayload = maxPayloadSize(type);
    if (size == -1) {
        // default size of -1, means biggest packet possible
        size = maxPayload;
    }
    
    // Fail with invalid size
    Q_ASSERT(size >= 0 || size < maxPayload);
    
    // allocate memory
    return std::unique_ptr<NLPacket>(new NLPacket(type, size));
}

std::unique_ptr<NLPacket> NLPacket::createCopy(const NLPacket& other) {
    return std::unique_ptr<NLPacket>(new NLPacket(other));
}

NLPacket::NLPacket(PacketType::Value type, qint64 size) : Packet(type, localHeaderSize(type) + size) {
}

NLPacket::NLPacket(const NLPacket& other) : Packet(other) {
}

void NLPacket::setSourceUuid(QUuid sourceUuid) {
    Q_ASSERT(!NON_SOURCED_PACKETS.contains(_type));
    auto offset = Packet::totalHeadersSize();
    memcpy(_packet.get() + offset, sourceUuid.toRfc4122().constData(), NUM_BYTES_RFC4122_UUID);
}

void NLPacket::setConnectionUuid(QUuid connectionUuid) {
    Q_ASSERT(!NON_VERIFIED_PACKETS.contains(_type));
    auto offset = Packet::totalHeadersSize() +
                    ((NON_SOURCED_PACKETS.contains(_type)) ? 0 : NUM_BYTES_RFC4122_UUID);
    memcpy(_packet.get() + offset, connectionUuid.toRfc4122().constData(), NUM_BYTES_RFC4122_UUID);
}

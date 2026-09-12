#pragma once
namespace gateway::domain::entities {
    enum class CollisionSignalType {
        Unknown,
        Exception,
        Run, 
        Stop
    };

    enum class TransferSignalType {
        Unknown,
        RequestTransfer,
        AcceptTransfer,
        StartTransfer,
        CompleteTransfer
    };
}
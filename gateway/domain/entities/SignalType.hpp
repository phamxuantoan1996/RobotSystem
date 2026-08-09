#pragma once
namespace gateway::domain::entities {
    enum CollisionSignalType {
        Exception,
        Run, 
        Stop
    };

    enum TransferSignalType {
        Unknown,
        RequestTransfer,
        AcceptTransfer,
        StartTransfer,
        CompleteTransfer
    };
}
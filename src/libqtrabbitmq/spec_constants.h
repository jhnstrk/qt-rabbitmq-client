#ifndef SPEC_CONSTANTS_H
#define SPEC_CONSTANTS_H

namespace qmq {

enum class Connection {
    ID_ = 10,
    Start = 10,
    StartOk = 11,
    Secure = 20,
    SecureOk = 21,
    Tune = 30,
    TuneOk = 31,
    Open = 40,
    OpenOk = 41,
    Close = 50,
    CloseOk = 51,
    Blocked = 60,
    Unblocked = 61,
};

enum class Channel {
    ID_ = 20,
    Open = 10,
    OpenOk = 11,
    Flow = 20,
    FlowOk = 21,
    Close = 40,
    CloseOk = 41,
};

enum class Exchange {
    ID_ = 40,
    Declare = 10,
    DeclareOk = 11,
    Delete = 20,
    DeleteOk = 21,
    Bind = 30,
    BindOk = 31,
    Unbind = 40,
    UnbindOk = 51,
};

enum class Queue {
    ID_ = 50,
    Declare = 10,
    DeclareOk = 11,
    Bind = 20,
    BindOk = 21,
    Purge = 30,
    PurgeOk = 31,
    Delete = 40,
    DeleteOk = 41,
    Unbind = 50,
    UnbindOk = 51,
};

enum class Basic {
    ID_ = 60,
    Qos = 10,
    QosOk = 11,
    Consume = 20,
    ConsumeOk = 21,
    Cancel = 30,
    CancelOk = 31,
    Publish = 40,
    Return = 50,
    Deliver = 60,
    Get = 70,
    GetOk = 71,
    GetEmpty = 72,
    Ack = 80,
    Nack = 120,
    Reject = 90,
    RecoverAsync = 100,
    Recover = 110,
    RecoverOk = 111,
};

enum class Confirm {
    ID_ = 85,
    Select = 10,
    SelectOk = 11,
};

enum class Tx {
    ID_ = 90,
    Select = 10,
    SelectOk = 11,
    Commit = 20,
    CommitOk = 21,
    Rollback = 30,
    RollbackOk = 31,
};

} // namespace qmq

#endif // SPEC_CONSTANTS_H

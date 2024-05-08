A client library for Rabbit-MQ for Qt applications.
# Development

## Git hooks

1. Install pre-commit using one of the methods described here: 
 https://pre-commit.com/
2. Run `pre-commit install`


## TODOs

- Improve tune.
- Pluggable authentication.
- When a connection is established, store connection parameters (max frame size, channel max) in client.
  - Enforce channel max. (Rabbit gives me 2047)
  - Enforce max frame size and test it (Rabbit gives 131072)
  - Check heartbeat is working.
- Handle closed channels (internal state)
- Load tests.
- Address clang warnings
- Basic QoS / ok
- Basic cancel / ok
- Basic return (received by client)
- Basic get / ok / get-empty
- Basic reject
- Basic recoverAsync / ok
- Tx messages (transactions).



Done

- ✓ Heartbeat frames.
- ✓ Producer
- ✓ Consumer

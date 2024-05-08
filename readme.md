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
  - Enforce MAX_MESSAGE_SIZE
  - Check heartbeat is working.
- Handle closed channels (internal state)
- Make flow messages actually stop / start flow.
- Load tests.
- Address clang warnings




Done

- ✓ Heartbeat frames.
- ✓ Producer
- ✓ Consumer

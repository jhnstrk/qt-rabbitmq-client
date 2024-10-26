A client library for Rabbit-MQ for Qt applications.
# Development

## Git hooks

1. Install pre-commit using one of the methods described here: 
 https://pre-commit.com/
2. Run `pre-commit install`


## TODOs

- Examples
- Improve tune.
  - Test channel max. (Rabbit gives me 2047)
  - Enforce max frame size and test it (Rabbit gives 131072)
  - Enforce MAX_MESSAGE_SIZE
- Pluggable authentication.
- Handle closed channels (internal state)
- Make flow messages actually stop / start flow.
- Load tests.
- Address clang warnings
- Windows build (export macros)


Done

- ✓ Heartbeat frames.
- ✓ Producer
- ✓ Consumer

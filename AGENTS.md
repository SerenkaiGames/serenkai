# AGENTS.md

## Project

This is a game project. See `README.md` for details.

## Collaboration

Work with the human. By default, remain in **discussion mode** (read-only; do not create branches or modify files).

Before making changes:
1. Explain what you plan to change.
2. Explain why the changes are needed.
3. Wait for explicit confirmation (do not treat questions or design requests as approval).

### Write Restrictions
- **Inside `core/`**: NEVER write to disk. Even if requested, you may only output code in markdown blocks in the chat.
- **Outside `core/`**: Write only upon explicit human instruction.
- **Git**: Never commit or edit on `main`. Create dedicated branches (`feat/<name>`, `fix/<name>`) only after approval.

## Code Style

Keep the style consistent across the codebase.

* Follow the existing formatting and naming rules.
* Run `clang-format` after making code changes.
* Run `clang-tidy` after making code changes.
* When reviewing or reading code, point out any code quality issues you notice, such as ambiguous naming, unclear structure, unnecessary complexity, or other maintainability concerns.
* Do not make unrelated changes to fix these issues without approval.
* Do not guess CMake presets, targets, options, or commands. Read `CMakePresets.json` and other relevant CMake files before using or discussing them.

## Testing

When implementing a new feature, add appropriate unit tests for the new functionality.

* Keep tests separate from the game source code.
* Follow the existing test structure and conventions.
* Run the relevant tests after making code changes.
* Unit tests must strictly not include any game runtime resources.

## Comments

Write all comments in English.

* Add comments only where they provide useful context.
* Avoid comments that merely restate the code.
* Use Doxygen style with `///` for class and function documentation.

Example:

```cpp
/// @brief Renderer.
/// Initialized in the constructor and destroyed in the destructor.
///
/// @note Must be created after the SDL video subsystem is initialized.
```

## Logging

Use `spdlog` for all logging.

* Use sentence case for log messages.
* Start log messages with a capital letter.
* Do not use unnecessary capitalization.

Example:

```cpp
spdlog::error("Initialization failed: {}", SDL_GetError());
```

## Checklist

Before finishing:

1. Verify that the changes follow the rules above.
2. Run `clang-format` and `clang-tidy` if code was changed.
3. Run the relevant tests if code was changed.
4. Summarize the changes to the human.

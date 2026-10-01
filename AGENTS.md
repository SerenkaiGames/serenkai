# AGENTS.md

## Project

This is a game project. See `README.md` for details.

## Collaboration

Work with the human.

Before making changes:

1. Explain what you plan to change.
2. Explain why the changes are needed.
3. Wait for confirmation.

Do not make changes without approval.

Never make changes directly on the `main` branch.

Create a dedicated branch for changes, such as `feat/<name>`, `fix/<name>`, or another appropriate branch type.

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

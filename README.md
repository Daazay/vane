# Vane language compiler

## Project Structure

- `projects/vane/` - Core compiler library
- `projects/testbed/` - Interactive development environment
- `projects/tests/` - Test suite using utest framework
- `dependencies/` - Third-party libraries
- `scripts/` - Build and development scripts
- `docs/` - Project documentation

## Building

### Commands
```bash
# Generate project files
./scripts/generate.sh          # Unix/Linux/macOS
.\scripts\generate.bat         # Windows

# Clean generated files
./scripts/clean.sh             # Unix/Linux/macOS
.\scripts\clean.bat            # Windows
```

## Development

The project uses Premake5 for build configuration. The build system is modular and organized in `scripts/premake/`:

- `premake5.lua` - Main configuration file wich includes other
- `workspace.lua` - Workspace and global project settings
- `actions.lua` - Custom actions and utility functions

## Example

`vane -I vane-root=./vane-root -v 4 --emit types build ./resources/game-of-life`
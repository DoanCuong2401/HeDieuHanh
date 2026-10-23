# Contribution Guidelines

## 1. Branch Strategy

- `main`: stable branch containing reviewed changes.
- `feature/<name>`: new features or documentation.
- `fix/<name>`: bug fixes.

Examples:
- `feature/fuzzer-design`
- `feature/coverage-buffer`
- `fix/syscall-dispatch`

Do not commit directly to `main`.

## 2. Commit Messages

Use a short prefix describing the type of change:

- `docs:` documentation changes
- `feat:` new functionality
- `fix:` bug fixes
- `test:` tests
- `refactor:` code restructuring

Examples:

- `docs: add syscall inventory`
- `feat: add syscall sequence generator`
- `test: add process syscall cases`

## 3. Pull Request Workflow

1. Create or switch to a feature branch.
2. Make changes related to one task.
3. Review the changes and run relevant tests.
4. Commit the changes.
5. Push the branch to the team repository.
6. Open a Pull Request targeting `main`.
7. Ask at least one other team member to review the changes.
8. Address review comments and merge after approval.

Do not merge changes that have unresolved review comments or known failing tests.

## 4. Code and Documentation

- Keep changes focused on the assigned task.
- Document important interfaces and assumptions.
- Do not commit passwords, access tokens, or other secrets.
- Include test evidence when changing kernel or fuzzer behavior.
- Avoid committing generated build artifacts unless explicitly required.

## 5. Team Coordination

- Each task should have an owner and a clear deliverable.
- Report blockers to the team as soon as possible.
- Coordinate changes to shared kernel files before editing them.
- Record completed work and test evidence in the project task board.

## 6. Initial Repository Setup

The team repository is:

https://github.com/DoanCuong2401/26134-syscall-fuzzer

The default integration branch is `main`. Feature branches should be pushed to the team repository and submitted through Pull Requests.

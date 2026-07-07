# Contributing

## Commit Style

Use [Conventional Commits](https://www.conventionalcommits.org/):

```
<type>: <short description>

<optional body>
```

Types:
- `feat` — new feature or user-facing change
- `fix` — bug fix
- `docs` — documentation only
- `chore` — build, tooling, CI, project setup
- `refactor` — code restructuring without behavior change

Keep commits **atomic**: one logical change per commit. Each commit should build and pass.

## Branch Naming

```
<type>/<short-description>
```

Examples:
- `feat/prerequisite-resolution`
- `feat/claude-provider`
- `docs/config-format`
- `chore/ci-setup`
- `refactor/split-main`

## Workflow

1. Branch from `dev` for feature/fix work.
2. Make atomic commits on your branch.
3. Open a PR targeting `dev`.
4. After review, squash-merge into `dev`.

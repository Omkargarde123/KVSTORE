# Git Commit Message Guide for KVSTORE

Use this format going forward: `type: short description`

## Types
- feat:     new feature
- fix:      bug fix
- docs:     documentation only
- refactor: code change that isn't a fix or feature
- test:     adding/fixing tests
- perf:     performance improvement
- chore:    build/config/tooling changes

## Good Examples (copy the style, not literally)
feat: implement O(1) LRU eviction using hashmap + doubly linked list
feat: add TTL expiry via min-heap for lazy key expiration
feat: build 8-thread worker pool for concurrent client handling
fix: resolve race condition in shared hashmap access
fix: prevent memory leak in connection cleanup
perf: reduce average latency to sub-2ms under load
test: add load test for 10k concurrent requests
docs: add architecture diagram and usage instructions to README
chore: add Dockerfile for containerized deployment
chore: add .gitignore for build artifacts

## Bad Examples (avoid these)
"update"
"fix"
"final commit"
"changes"
"asdf"

## If your history already has bad commit messages
You don't need to rewrite old history (risky once pushed). Instead:
1. Going forward, use proper messages for every new commit
2. Optionally, do ONE clean "squash" before showing recruiters:
   git checkout -b clean-history
   git reset --soft <first-commit-hash>
   git commit -m "feat: initial KVSTORE implementation with LRU, TTL, and threading"
   (Only do this if you're comfortable with git — ask me if unsure)

# ── Build stage ──────────────────────────────
FROM gcc:13 AS builder
WORKDIR /app
COPY . .
RUN make kvstore kvstore-cli

# ── Runtime stage (slim) ─────────────────────
FROM debian:bookworm-slim
WORKDIR /app
COPY --from=builder /app/kvstore      ./kvstore
COPY --from=builder /app/kvstore-cli  ./kvstore-cli

EXPOSE 6379

# Default: port 6379, 8 threads, 1000 LRU capacity
CMD ["./kvstore", "6379", "8", "1000"]

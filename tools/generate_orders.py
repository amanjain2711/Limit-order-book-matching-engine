#!/usr/bin/env python3
"""Synthetic market event generator for NANOMATCH benchmarks."""

import argparse
import csv
import random
from pathlib import Path


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate synthetic order CSV")
    parser.add_argument("-n", "--count", type=int, default=100_000, help="number of events")
    parser.add_argument("-o", "--output", type=Path, default=Path("data/orders.csv"))
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--cancel-rate", type=float, default=0.05)
    parser.add_argument("--market-rate", type=float, default=0.10)
    args = parser.parse_args()

    rng = random.Random(args.seed)
    args.output.parent.mkdir(parents=True, exist_ok=True)

    base_price = 1_000_000
    open_orders: list[int] = []
    next_id = 1

    with args.output.open("w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["type", "id", "side", "price", "qty", "ts"])

        for ts in range(args.count):
            roll = rng.random()
            if open_orders and roll < args.cancel_rate:
                oid = rng.choice(open_orders)
                open_orders.remove(oid)
                writer.writerow(["CANCEL", oid, "BUY", 0, 0, ts])
                continue

            if roll < args.cancel_rate + args.market_rate:
                side = rng.choice(["BUY", "SELL"])
                qty = rng.randint(1, 50)
                writer.writerow(["MARKET", next_id, side, 0, qty, ts])
                next_id += 1
                continue

            side = rng.choice(["BUY", "SELL"])
            price = base_price + rng.randint(-2000, 2000)
            qty = rng.randint(1, 100)
            writer.writerow(["LIMIT", next_id, side, price, qty, ts])
            open_orders.append(next_id)
            next_id += 1

    print(f"Wrote {args.count} events to {args.output}")


if __name__ == "__main__":
    main()

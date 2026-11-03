#!/usr/bin/env python3
"""Fragmented echo peer for Atari STREAMTEST. No public services needed."""
import argparse
import asyncio
import random

async def client(reader, writer):
    rng = random.Random(12345)
    total = 0
    print('connected', writer.get_extra_info('peername'), flush=True)
    try:
        while data := await reader.read(120):
            total += len(data)
            while data:
                n = rng.randint(1, len(data))
                await asyncio.sleep(rng.uniform(0, 0.025))
                writer.write(data[:n])
                await writer.drain()
                data = data[n:]
    finally:
        writer.close()
        await writer.wait_closed()
        print('closed; echoed', total, 'bytes', flush=True)

async def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--host', default='127.0.0.1')
    p.add_argument('--port', default=9004, type=int)
    a = p.parse_args()
    server = await asyncio.start_server(client, a.host, a.port)
    print(f'Listening on {a.host}:{a.port}', flush=True)
    async with server:
        await server.serve_forever()

if __name__ == '__main__':
    asyncio.run(main())

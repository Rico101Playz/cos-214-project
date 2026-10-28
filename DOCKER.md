# Running TIVIDY in Docker

Put these files in the root of the repo (next to `CMakeLists.txt`): `Dockerfile`, `docker-compose.yml`, `.dockerignore`, `.gitattributes` and the `docker/` folder.

You need Docker Desktop (Windows or macOS) or Docker Engine (Linux), running.

## Start it

From the repo folder, in PowerShell, Git Bash or any terminal:

```
git switch demo
docker compose up -d --build
```

The first build takes a few minutes. It compiles everything and runs all 117 tests, and the build stops if a test fails.

Then open these, one person per browser window:

```
http://localhost:8080
http://localhost:8081
http://localhost:8082
```

## Everyday commands

| What | Command |
| --- | --- |
| See the server output | `docker compose logs -f` |
| Stop it (keeps your data) | `docker compose down` |
| Start it again | `docker compose up -d` |
| Rebuild after pulling new code | `docker compose up -d --build` |
| Start over with the clean demo company | `docker compose down -v` then `docker compose up -d` |
| Use the console version | `docker compose exec tividy /app/bin/TIVIDY_App --scenario /data/acme-software.txt` (stop using the website while you do) |

## How it works

* The image is built in two stages. The first compiles the project and runs the tests, the second only holds the server, the website and the demo scenario.
* Your company is saved in a Docker volume called `tividy-data`, so it survives restarts and rebuilds. `docker compose down -v` deletes it.
* The server only accepts connections from the same machine. Inside the container it listens on 18080 to 18082, and a small forwarder (socat) passes ports 8080 to 8082 through to it. The ports are published on `127.0.0.1` only, so nobody else on your network can reach it. The server log mentions 18080 to 18082, but always use 8080 to 8082 in the browser.
* To use another scenario name, change `TIVIDY_SCENARIO` in `docker-compose.yml`. If that file does not exist yet, it is created from the demo company.

## Without Compose

```
docker build -t tividy .
docker run -d --name tividy -p 127.0.0.1:8080:8080 -p 127.0.0.1:8081:8081 -p 127.0.0.1:8082:8082 -v tividy-data:/data tividy
```

Stop and remove it with `docker rm -f tividy`.

## If something goes wrong

| Problem | Fix |
| --- | --- |
| `entrypoint.sh: not found` or `bad interpreter` | The script got Windows line endings. The `.gitattributes` file prevents that, and the Dockerfile also strips them, so rebuild with `docker compose build --no-cache` |
| Port already in use | Stop the server you started outside Docker, or change the left-hand numbers in `docker-compose.yml`, for example `127.0.0.1:9080:8080` |
| The page loads but says "Connecting to the TIVIDY server..." | Open it as `http://localhost:8080`, not by your computer's network address. The server only allows local pages |

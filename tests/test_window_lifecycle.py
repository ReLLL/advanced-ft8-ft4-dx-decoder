"""Check a running Brown instance through its existing local module test API."""
import argparse
import json
import time
import urllib.parse
import urllib.request


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--url", default="http://127.0.0.1:8080")
    args = parser.parse_args()
    if urllib.parse.urlparse(args.url).hostname not in ("127.0.0.1", "localhost", "::1"):
        raise ValueError("Use an existing local test endpoint")
    endpoint = args.url.rstrip("/") + "/module/" + urllib.parse.quote("Kondor Advanced Decoder", safe="") + "/command"

    def command(name, values=""):
        body = json.dumps({"cmd": name, "args": values}).encode()
        request = urllib.request.Request(endpoint, body, {"Content-Type": "application/json"})
        with urllib.request.urlopen(request, timeout=8) as response:
            return json.load(response)

    def wait_for(predicate, seconds=10):
        deadline = time.monotonic() + seconds
        while time.monotonic() < deadline:
            state = command("window_status")
            if predicate(state):
                return state
            time.sleep(0.25)
        raise AssertionError("Window lifecycle condition timed out")

    before = command("window_status")
    assert before["enabled"] and not before["paused"]
    receiver = command("status")
    assert receiver["channels"], "Tune a supported dial inside the RF capture"
    try:
        command("hide")
        hidden = wait_for(lambda s: not s["open"] and s["enabled"] and s["messages"] > before["messages"] and s["uptime_seconds"] > before["uptime_seconds"] + 3, 60)
        work = command("status")
        assert work["jobs"] > receiver["jobs"]
        assert sum(c["samples"] for c in work["channels"]) > sum(c["samples"] for c in receiver["channels"])
        assert hidden["table_cleared"] == before["table_cleared"] and hidden["map_cleared"] == before["map_cleared"]
        assert hidden["corner_visible"] and hidden["corner_messages"] > 0
        command("show")
        reopened = wait_for(lambda s: s["open"] and s["generation"] > hidden["generation"] and s["messages"] >= hidden["messages"])
        assert reopened["position"] == before["position"] and reopened["size"] == before["size"]
        for tab, flag in ((1, "table_messages"), (2, "table_countries")):
            command("view", json.dumps({"table_tab": tab}))
            wait_for(lambda s: s["table_tab"] == tab and s[flag] and s["table_rows"] > 0)
        command("corner_notifications", "0")
        wait_for(lambda s: not s["corner_enabled"] and not s["corner_visible"])
        command("corner_notifications", "1")
        wait_for(lambda s: s["corner_enabled"] and s["corner_visible"])
        print(json.dumps({"hidden_new_receptions": hidden["messages"] - before["messages"], "hidden_uptime_gain": hidden["uptime_seconds"] - before["uptime_seconds"], "window_geometry_preserved": True, "all_messages_and_countries_tabs": True, "corner_toggle": True}))
    finally:
        command("view", json.dumps({"table_tab": before["table_tab"]}))
        command("corner_notifications", "1" if before["corner_enabled"] else "0")
        command("show" if before["open"] else "hide")


if __name__ == "__main__":
    main()

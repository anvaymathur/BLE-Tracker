
import { useState, useEffect } from "react";
import "./App.css";
import { watchGateways } from "./firebase.js";

function App() {
  const [gateways, setGateways] = useState({});

  // Gateway locations for the future map.
  const RECEIVER_LOCATIONS = {
    A01: ["Aisle 1"],
    A02: ["Aisle 2"],
    A03: ["Aisle 3"],
    A04: ["Aisle 4"],
  };

  useEffect(() => {
    const unsubscribe = watchGateways((data) => {
      setGateways(data ?? {});
    });

    return () => unsubscribe();
  }, []);

  const gatewayEntries = Object.entries(gateways);

  return (
    <main id="center">
      <h1>BLE Tracker</h1>

      <h2>Gateways: {gatewayEntries.length}</h2>

      {gatewayEntries.length === 0 ? (
        <p>No gateways found.</p>
      ) : (
        gatewayEntries.map(([gatewayID, tags]) => (
          <section key={gatewayID} className="gateway">
            <h2>Gateway: {gatewayID}</h2>

            <p>
              Location:{" "}
              {RECEIVER_LOCATIONS[gatewayID]?.[0] ?? "Unknown"}
            </p>

            {!tags || Object.keys(tags).length === 0 ? (
              <p>No tags detected.</p>
            ) : (
              Object.entries(tags).map(([tagID, readings]) => (
                <article key={tagID} className="tag">
                  <h3>Tag: {tagID}</h3>

                  <p>
                    RSSI: {readings?.rssi ?? "N/A"} dBm
                  </p>

                  <p>
                    Last seen: {readings?.last_seen ?? "N/A"}
                  </p>
                </article>
              ))
            )}
          </section>
        ))
      )}
    </main>
  );
}

export default App;

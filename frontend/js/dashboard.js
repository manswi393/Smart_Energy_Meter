async function fetchData() {
  try {
    const res = await fetch("http://localhost:3000/data");
    const data = await res.json();

    document.getElementById("power").innerText =
      data.power.toFixed(2) + " W";

    document.getElementById("current").innerText =
      data.current.toFixed(2) + " A";

    document.getElementById("energy").innerText =
      data.energy.toFixed(4) + " kWh";

    document.getElementById("prediction").innerText =
      "Rs " + data.prediction.toFixed(0);

  } catch (err) {
    console.log("Error fetching data");
  }
}

setInterval(fetchData, 1000);
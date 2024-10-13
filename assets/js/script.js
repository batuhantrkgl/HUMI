const devMode = true; // Enable developer mode
const timestamps = [];
const humidityData = [];
const alertHistory = []; // Keeps all alerts
const careTips = []; // Store user-generated tips
const colors = [
  "rgba(75, 192, 192, 1)",
  "rgba(255, 99, 132, 1)",
  "rgba(255, 206, 86, 1)",
  "rgba(54, 162, 235, 1)",
  "rgba(255, 159, 64, 1)",
];

const themeToggle = document.getElementById("theme-toggle");
let currentTheme = localStorage.getItem("theme");

// Detect system theme and set it
const systemTheme = window.matchMedia("(prefers-color-scheme: dark)").matches
  ? "dark"
  : "light";
if (!currentTheme) {
  currentTheme = systemTheme; // Use system theme if no preference is set
}

function updateToggleIcon(theme) {
  if (theme === "dark") {
    themeToggle.innerHTML = '<i class="fas fa-sun"></i>'; // Sun icon for light mode
  } else {
    themeToggle.innerHTML = '<i class="fas fa-moon"></i>'; // Moon icon for dark mode
  }
}

document.body.classList.add(currentTheme);
updateToggleIcon(currentTheme); // Set initial icon

if (window.matchMedia("(max-width: 600px)").matches) {
  // Do not attach event listener for mobile
} else {
  themeToggle.addEventListener("click", () => {
    currentTheme = currentTheme === "dark" ? "light" : "dark";
    document.body.className = currentTheme;
    localStorage.setItem("theme", currentTheme);
    updateToggleIcon(currentTheme);
  });
}

// Listen for system theme changes
window
  .matchMedia("(prefers-color-scheme: dark)")
  .addEventListener("change", (e) => {
    currentTheme = e.matches ? "dark" : "light";
    document.body.className = currentTheme;
    localStorage.setItem("theme", currentTheme);
    updateToggleIcon(currentTheme);
  });
// Save dashboard layout
function saveLayout() {
  const layout = [...document.querySelectorAll(".widget")].map(
    (widget) => widget.id
  );
  localStorage.setItem("dashboardLayout", JSON.stringify(layout));
}

// Load dashboard layout
function loadLayout() {
  const layout = JSON.parse(localStorage.getItem("dashboardLayout"));
  if (layout) {
    const dashboard = document.getElementById("dashboard");
    layout.forEach((id) => {
      const widget = document.getElementById(id);
      dashboard.appendChild(widget); // Reorder widgets
    });
  }
}

const dashboard = document.getElementById("dashboard");
let draggedWidget = null;

dashboard.addEventListener("dragstart", (e) => {
  if (e.target.classList.contains("widget")) {
    draggedWidget = e.target;
    e.target.style.opacity = 0.5; // Reduce opacity of dragged widget
  }
});

dashboard.addEventListener("dragend", (e) => {
  e.target.style.opacity = ""; // Reset opacity
  draggedWidget = null;
  removeDropTargets(); // Clear drop targets
});

dashboard.addEventListener("dragover", (e) => {
  e.preventDefault();
  if (e.target.classList.contains("widget")) {
    e.target.classList.add("drop-target"); // Highlight drop target
  }
});

dashboard.addEventListener("dragleave", (e) => {
  if (e.target.classList.contains("widget")) {
    e.target.classList.remove("drop-target"); // Remove highlight
  }
});

dashboard.addEventListener("drop", (e) => {
  e.preventDefault();
  if (e.target.classList.contains("widget") && draggedWidget) {
    dashboard.insertBefore(draggedWidget, e.target.nextSibling);
    saveLayout(); // Save layout on drop
  }
  removeDropTargets(); // Clear drop targets
});

function removeDropTargets() {
  const dropTargets = document.querySelectorAll(".drop-target");
  dropTargets.forEach((target) => {
    target.classList.remove("drop-target"); // Remove highlight
  });
}

function submitTip() {
  const tipInput = document.getElementById("tip-input");
  const tip = tipInput.value.trim();
  if (tip) {
    careTips.push(tip);
    tipInput.value = "";
    updateTipsList();
  }
}

function updateTipsList() {
  const tipsList = document.getElementById("tips-list");
  tipsList.innerHTML = ""; // Clear previous tips
  careTips.forEach((tip) => {
    const tipItem = document.createElement("div");
    tipItem.textContent = tip;
    tipsList.appendChild(tipItem);
  });
}

function fetchPlantCareTips() {
  if (devMode) {
    // Simulate SD card data
    const simulatedSDData = generateSimulatedSDData();
    sendToGeminiAPI(simulatedSDData);
  } else {
    fetch("http://192.168.1.100/plant-care-tips")
      .then(response => response.text())
      .then(data => {
        document.getElementById("gemini-response").innerText = data;
      })
      .catch(error => console.error("Error:", error));
  }
}

function generateSimulatedSDData() {
  const data = [];
  for (let i = 0; i < 10; i++) {
    const humidity = Math.floor(Math.random() * 100);
    const timestamp = new Date().toISOString();
    data.push({ timestamp, humidity });
  }
  return JSON.stringify(data);
}

function sendToGeminiAPI(data) {
  fetch('http://localhost:3000/generateContent', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
    },
    body: JSON.stringify({ contents: [{ parts: [{ text: data }] }] })
  })
    .then(response => response.json())
    .then(data => {
      const geminiResponse = data.candidates[0].content.parts[0].text;
      document.getElementById("gemini-response").innerHTML = marked.parse(geminiResponse);
    })
    .catch(error => console.error("Error:", error));
}

// Existing functions
function waterPlant(plantNumber) {
  if (devMode) {
    alert(`Simulated: Watering Plant ${plantNumber}`);
  } else {
    fetch(`http://192.168.1.100/water?plant=${plantNumber}`)
      .then((response) => response.text())
      .then((data) => alert(data))
      .catch((error) => console.error("Error:", error));
  }
}

function toggleFan(plantNumber) {
  if (devMode) {
    alert(`Simulated: Toggling Fan for Plant ${plantNumber}`);
  } else {
    fetch(`http://192.168.1.100/fan?plant=${plantNumber}`)
      .then((response) => response.text())
      .then((data) => alert(data))
      .catch((error) => console.error("Error:", error));
  }
}

function takePhoto() {
  alert("Fotoğraf çekme özelliği henüz eklenmedi.");
}

function updateHumidity() {
  const currentTime = new Date().toLocaleTimeString();
  const plantList = document.getElementById("plant-list");
  const scrollPosition = plantList.scrollTop;

  if (devMode) {
    const mockData = [];
    for (let i = 0; i < colors.length; i++) {
      const humidity = Math.floor(Math.random() * 100);
      mockData.push({ humidity: humidity, recommendedHumidity: 60 });
      if (!humidityData[i]) {
        humidityData[i] = [];
      }
      humidityData[i].push(humidity);
      checkAndLogAlert(i, humidity);
    }
    renderPlants(mockData);
    timestamps.push(currentTime);
    updateChart(false);
  } else {
    fetch("http://192.168.1.100/humidity")
      .then((response) => response.json())
      .then((data) => {
        renderPlants(data);
        data.forEach((plant, index) => {
          if (!humidityData[index]) {
            humidityData[index] = [];
          }
          humidityData[index].push(plant.humidity);
          checkAndLogAlert(index, plant.humidity);
        });
        timestamps.push(currentTime);
        updateChart(false);
      })
      .catch((error) => console.error("Error:", error));
  }

  plantList.scrollTop = scrollPosition;
}

function renderPlants(data) {
  const plantList = document.getElementById("plant-list");
  plantList.innerHTML = "";

  data.forEach((plant, index) => {
    const plantContainer = document.createElement("div");
    plantContainer.classList.add("team-member-container");

    const humidity = plant.humidity;
    const recommended = plant.recommendedHumidity;
    const alertMessage = checkHumidity(index, humidity, recommended);

    if (alertMessage) {
      plantContainer.classList.add("alert");
    }

    const plantInfo = `
    <div class="team-member-info">
        <p class="teammate-name">Bitki ${index + 1}</p>
        <p>Nem Durumu: <span id="humidity-${index + 1}">${humidity}%</span></p>
        <p>Önerilen Nem: <span>${recommended}%</span></p>
        ${alertMessage ? `<p class="alert">${alertMessage}</p>` : ""}
        <div class="button-container">
            <button class="action-button" onclick="takePhoto()">
                <i class="fas fa-camera"></i>
            </button>
            <button class="action-button" onclick="waterPlant(${index + 1})">
                <i class="fas fa-water"></i>
            </button>
            <button class="action-button" onclick="toggleFan(${index + 1})">
                <i class="fas fa-fan"></i>
            </button>
        </div>
    </div>
`;
    plantContainer.innerHTML = plantInfo;
    plantList.appendChild(plantContainer);
  });
}

function checkHumidity(index, humidity, recommended) {
  if (humidity < recommended - 10 || humidity > recommended + 10) {
    return `Uyarı: Bitki ${index + 1} nem seviyesi uygun değil! (${humidity}%)`;
  }
  return "";
}

function checkAndLogAlert(index, humidity) {
  const recommendedHumidity = 60; // Replace with dynamic value if needed
  if (
    humidity < recommendedHumidity - 10 ||
    humidity > recommendedHumidity + 10
  ) {
    const alertMessage = `Bitki ${index + 1}: Nem seviyesi ${humidity}%`;
    alertHistory.unshift(alertMessage); // Add the new alert at the beginning
    if (alertHistory.length > 10) {
      alertHistory.pop(); // Keep only the last 10 alerts
    }
    updateAlertHistory();
  }
}

function updateAlertHistory() {
  const alertList = document.getElementById("alert-list");
  alertList.innerHTML = ""; // Clear previous alerts
  alertHistory.forEach((alert) => {
    const alertItem = document.createElement("div");
    alertItem.className = "alert-container";
    alertItem.textContent = alert;
    alertItem.onclick = () => deleteAlert(alert);
    alertList.appendChild(alertItem);
  });
}

function deleteAlert(alertMessage) {
  const index = alertHistory.indexOf(alertMessage);
  if (index > -1) {
    alertHistory.splice(index, 1);
    updateAlertHistory(); // Update displayed alerts
  }
}

function updateChart(isInitialLoad) {
  const ctx = document.getElementById("humidityChart").getContext("2d");

  if (window.humidityChart instanceof Chart) {
    window.humidityChart.destroy();
  }

  const datasets = humidityData.map((data, index) => ({
    label: `Plant ${index + 1}`,
    data: data,
    borderColor: colors[index % colors.length],
    backgroundColor: colors[index % colors.length].replace("1)", "0.2)"),
    fill: true,
  }));

  window.humidityChart = new Chart(ctx, {
    type: "line",
    data: {
      labels: timestamps,
      datasets: datasets,
    },
    options: {
      animations: isInitialLoad
        ? {
            tension: {
              duration: 500,
              easing: "linear",
              from: 1,
              to: 0,
              loop: false,
            },
          }
        : false,
      scales: {
        y: {
          beginAtZero: true,
          title: {
            display: true,
            text: "Humidity (%)",
          },
        },
        x: {
          title: {
            display: true,
            text: "Time",
          },
        },
      },
    },
  });
}

document.addEventListener("DOMContentLoaded", function () {
  loadLayout(); // Load layout on page load
  const days = [
    "Sunday",
    "Monday",
    "Tuesday",
    "Wednesday",
    "Thursday",
    "Friday",
    "Saturday",
  ];
  const today = new Date();
  document.getElementById("current-day").textContent = days[today.getDay()];

  setInterval(updateHumidity, 3000); // Update every 3 seconds
  updateHumidity(); // Initial load
  fetchPlantCareTips(); // Fetch plant care tips on page load
});
// Automatically follow system theme on mobile
if (
  window.matchMedia &&
  window.matchMedia("(prefers-color-scheme: dark)").matches
) {
  document.body.classList.add("dark");
} else {
  document.body.classList.add("light");
}
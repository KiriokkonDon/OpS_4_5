const url = new URL(window.location.href);
const log_type = url.searchParams.get("log");

const h1_map = {
    "all": "Temperature all",
    "hour": "Temperature hour",
    "day": "Temperature day"
};

if (!h1_map[log_type]) {
    window.location.replace(`${url.origin}/404.http`);
}

fetch(`/${log_type}`)
    .then(response => response.ok ? response.text() : Promise.reject(response))
    .then(data => {
        const table = document.querySelector(".tab");
        table.innerHTML = `<tr><th>Time</th><th>Temperature</th></tr>` +
            data.trim().split("\n").map(line => {
                const [timestamp, temp] = line.split(" ");
                const date = new Date(parseInt(timestamp) * 1000).toLocaleString();
                return `<tr><td>${date}</td><td>${temp}</td></tr>`;
            }).join("");
    })
    .catch(() => {
        document.querySelector(".tab").innerHTML = "No data gathered yet!";
    })
    .finally(() => {
        document.querySelector("h1").textContent = h1_map[log_type];
    });

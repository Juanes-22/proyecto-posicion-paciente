// DOM elements via jQuery
const $value_text = $("#value-text");
const $seconds_text = $("#seconds-text");
const $timer_text = $("#timer-text");
const $device_text = $("#device-text");

// Ubidots parameters
const TOKEN = "";
const VAR_ID = "";

let lastValue = null; // para almacenar value del último dot de la variable
let deviceActive = false; // para indicar si el device está activo o no
let gotSeconds = false; // para indicar si se obtuvieron los segundos al refrezcar la página y device activo
let firstSeconds = false;

// Timer variables
let timeCount = 0;
let timerId = null;
let timerStarted = false;

// Will only run once the page DOM is ready for JavaScript code to execute
$(document).ready(function () {
    setInterval(() => {
        getLastDot(VAR_ID, TOKEN);
    }, 1500);
});

function getLastDot(variableId, token) {
    let url =
        "https://industrial.api.ubidots.com/api/v1.6/variables/" +
        variableId +
        "/values";

    $.ajax({
        method: "GET",
        url: url,
        data: { token: token, page_size: 1 },
        success: function (res) {
            let currentValue = res.results[0].value; // value del último dot de la variable
            let currentSeconds = res.results[0].context.seconds; // extraído del context del último dot de la variable
            let currentMilliseconds = res.results[0].timestamp; // extraído del timestamp del último dot de la variable

            // actualiza texto del DOM
            $value_text.text(currentValue);

            if (!firstSeconds) {
                $seconds_text.text(currentSeconds);
                firstSeconds = true;
            }

            // si han transcurrido más de 3s desde último dot, el device está inactivo
            if (Date.now() - currentMilliseconds > 3000) {
                deviceActive = false;
                $device_text.text("El device está inactivo");
                $seconds_text.text(currentSeconds);
            } else {
                deviceActive = true;
                $device_text.text("El device está activo");
            }

            // al refrezcar la página y device activo, cargue últimos segundos y startTimer()
            if (deviceActive && !gotSeconds) {
                timeCount = currentSeconds;
                gotSeconds = true;
                startTimer();
                timerStarted = true;
            } else if (deviceActive && !timerStarted) {
                startTimer();
                timerStarted = true;
            } else if (!deviceActive) {
                stopTimer();
                timerStarted = false;
            }

            // mientras corre el timer, verifica si cambió el value
            if (timerStarted) {
                if (lastValue != null && lastValue != currentValue) {
                    stopTimer();
                    timerStarted = false;
                }
            }
            lastValue = currentValue;
        },
    });
}

function startTimer() {
    timerId = setInterval(() => {
        updateTimerText();
        timeCount++;
    }, 1000);
}

function stopTimer() {
    timeCount = 0;
    updateTimerText();
    clearInterval(timerId);
}

function updateTimerText() {
    let hours = Math.floor(timeCount / 3600);
    let minutes = Math.floor(timeCount / 60) % 60;
    let seconds = timeCount % 60;

    seconds = seconds < 10 ? "0" + seconds : seconds;
    minutes = minutes < 10 ? "0" + minutes : minutes;
    hours = hours < 10 ? "0" + hours : hours;

    $timer_text.text(`${hours}:${minutes}:${seconds}`);
}

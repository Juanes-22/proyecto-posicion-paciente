var $bg = $("#image-container");
var $text = $("#value-text");

var TOKEN = "";
var VAR_ID = "";
var lastValue = null;

function changeImage(value) {
    var choose = "supine";
    var background = {
        supine: "https://nurseslabs.com/wp-content/uploads/2022/06/SUPINE-DORSAL-RECUMBENT-PATIENT-POSITIONING-GUIDE-AND-CHEAT-SHEET.jpg",
        prone: "https://nurseslabs.com/wp-content/uploads/2022/06/PRONE-PATIENT-POSITIONING-GUIDE-AND-CHEAT-SHEET.jpg",
        lateral: "https://nurseslabs.com/wp-content/uploads/2022/06/LATERAL-PATIENT-POSITIONING-GUIDE-AND-CHEAT-SHEET.jpg",
    };

    if (value == 1) {
        choose = "supine";
    } else if (value == 2) {
        choose = "prone";
    } else if (value == 3 || value == 4) {
        choose = "lateral";
    }

    $bg.css("background-image", "url(" + background[choose] + ")");
}

function getLastValue(variableId, token) {
    var url =
        "https://industrial.api.ubidots.com/api/v1.6/variables/" +
        variableId +
        "/values";

    $.get(url, { token: token, page_size: 1 }, function (res) {
        if (lastValue === null || res.results[0].value !== lastValue) {
            lastValue = res.results[0].value;
            $text.text(lastValue);
            changeImage(lastValue);
        }
    });
}

setInterval(() => {
    getLastValue(VAR_ID, TOKEN);
}, 2000);

(function ($) {

    'use strict';

    function SigV4Utils() { }

    SigV4Utils.sign = function (key, msg) {
        var hash = CryptoJS.HmacSHA256(msg, key);
        return hash.toString(CryptoJS.enc.Hex);
    };

    SigV4Utils.sha256 = function (msg) {
        var hash = CryptoJS.SHA256(msg);
        return hash.toString(CryptoJS.enc.Hex);
    };

    SigV4Utils.getSignatureKey = function (key, dateStamp, regionName, serviceName) {
        var kDate = CryptoJS.HmacSHA256(dateStamp, 'AWS4' + key);
        var kRegion = CryptoJS.HmacSHA256(regionName, kDate);
        var kService = CryptoJS.HmacSHA256(serviceName, kRegion);
        var kSigning = CryptoJS.HmacSHA256('aws4_request', kService);
        return kSigning;
    };

    function createEndpoint(regionName, awsIotEndpoint, accessKey, secretKey) {
        var time = moment.utc();
        var dateStamp = time.format('YYYYMMDD');
        var amzdate = dateStamp + 'T' + time.format('HHmmss') + 'Z';
        var service = 'iotdevicegateway';
        var region = regionName;
        var secretKey = secretKey;
        var accessKey = accessKey;
        var algorithm = 'AWS4-HMAC-SHA256';
        var method = 'GET';
        var canonicalUri = '/mqtt';
        var host = awsIotEndpoint;

        var credentialScope = dateStamp + '/' + region + '/' + service + '/' + 'aws4_request';
        var canonicalQuerystring = 'X-Amz-Algorithm=AWS4-HMAC-SHA256';
        canonicalQuerystring += '&X-Amz-Credential=' + encodeURIComponent(accessKey + '/' + credentialScope);
        canonicalQuerystring += '&X-Amz-Date=' + amzdate;
        canonicalQuerystring += '&X-Amz-SignedHeaders=host';

        var canonicalHeaders = 'host:' + host + '\n';
        var payloadHash = SigV4Utils.sha256('');
        var canonicalRequest = method + '\n' + canonicalUri + '\n' + canonicalQuerystring + '\n' + canonicalHeaders + '\nhost\n' + payloadHash;

        var stringToSign = algorithm + '\n' + amzdate + '\n' + credentialScope + '\n' + SigV4Utils.sha256(canonicalRequest);
        var signingKey = SigV4Utils.getSignatureKey(secretKey, dateStamp, region, service);
        var signature = SigV4Utils.sign(signingKey, stringToSign);

        canonicalQuerystring += '&X-Amz-Signature=' + signature;
        return 'wss://' + host + canonicalUri + '?' + canonicalQuerystring;
    }

    var endpoint = createEndpoint(
        'eu-west-2', // Your Region
        'XXX', // AWS IoT endpoint
        'XXX', // SecretKey
        'XXX'); // AccessKey
    var clientId = Math.random().toString(36).substring(7);
    var client = new Paho.MQTT.Client(endpoint, clientId);
    var connectOptions = {
        useSSL: true,
        timeout: 3,
        mqttVersion: 4,
        onSuccess: subscribe
    };
    client.onMessageArrived = onMessage;
    client.onConnectionLost = function (e) { console.log(e) };

    document.getElementById('send').addEventListener('click', function (e) {
        var say = document.getElementById('say')
        send(say.value);
        say.value = '';
    });

    document.getElementById('connect').addEventListener('click', function (e) {
        client.connect(connectOptions);
        console.log("Connected");
    });
    document.getElementById('disconnect').addEventListener('click', function (e) {
        client.disconnect();
        console.log("Disconnected");
    });
    document.getElementById('test').addEventListener('click', function (e) {
        document.getElementById('da0').innerHTML = payload.join(' ');
        document.getElementById('da1').innerHTML = eeg_data1.join(' ');
        document.getElementById('da2').innerHTML = eeg_data2.join(' ');
        document.getElementById('da3').innerHTML = eeg_data3.join(' ');

    });
    document.getElementById('graph').addEventListener('click', function (e) {
        update();
    });
    document.getElementById('play').addEventListener('click', function (e) {
        if (game_buf[0] == 0) {
            Podium.keydown(38);
        } else if (game_buf[0] == 1) {
            Podium.keydown(37);
        } else if (game_buf[0] == 2) {
            Podium.keydown(39);
        }
        game_buf = game_buf.slice(1);
        console.log(game_buf.length);
    });

    var Podium = {};
    Podium.keydown = function (k) {
        var oEvent = document.createEvent('KeyboardEvent');

        // Chromium Hack
        Object.defineProperty(oEvent, 'keyCode', {
            get: function () {
                return this.keyCodeVal;
            }
        });
        Object.defineProperty(oEvent, 'which', {
            get: function () {
                return this.keyCodeVal;
            }
        });

        if (oEvent.initKeyboardEvent) {
            oEvent.initKeyboardEvent("keydown", true, true, document.defaultView, false, false, false, false, k, k);
        } else {
            oEvent.initKeyEvent("keydown", true, true, document.defaultView, false, false, false, false, k, 0);
        }

        oEvent.keyCodeVal = k;

        if (oEvent.keyCode !== k) {
            alert("keyCode mismatch " + oEvent.keyCode + "(" + oEvent.which + ")");
        }

        document.dispatchEvent(oEvent);
    }

    function calculate(array) {
        var total = 0;
        for (var i = 0; i < array.length; i++) {
            total += array[i];
        }
        var avg = total / array.length;
        return avg;
    }


    function subscribe() {
        var subscribeOptions = {
            //onSuccess: update
        };
        client.subscribe("esp32/+", subscribeOptions);
        console.log("subscribed");
    }

    function send(content) {
        var message = new Paho.MQTT.Message(content);
        message.destinationName = "esp32/1";
        client.send(message);
        console.log("sent");
    }
    function onMessage(message) {
        unpack(message);
        //extra();
    }

    function byteArrayToIntArray(byteArray) {
        var intArray = new Array(10);
        var j = 0;
        for (var i = 0; i < byteArray.length; i = i + 2) {
            var byteA = byteArray[i];
            var byteB = byteArray[i + 1];
            var sign = byteB & (1 << 7);
            var x = (((byteB & 0xFF) << 8) | (byteA & 0xFF));
            var result;
            if (sign) {
                result = 0xFFFF0000 | x;  // fill in most significant bits with 1's
            } else {
                result = x;
            }
            intArray[j] = result;
            j = j + 1;
        }
        return intArray;
    }

    function normalize(array) {
        var arr = new Array(10);
        for (let i = 0; i < array.length; i++) {
            arr[i] = 2 * (array[i] - Math.min.apply(Math, array)) / (Math.max.apply(Math, array) - Math.min.apply(Math, array)) - 1;
        }
        return arr;
    }
    function getUint64(bytes) {
        var shift = 1;
        var res = 0;
        for (let i = 0; i < bytes.length; i++) {
            res += bytes[i] * shift;
            shift *= 256;
        }
        return res;
    }

    window.game_buf = [];
    var cnt = 0;
    var arr = new Array(1);
    var arr1 = new Array(0);
    function unpack(message) {
        //console.log("Timestamp when the data was received: " + Date.now());
        var payload = message.payloadBytes;
        var length = payload.length;
        var time_stamp = getUint64(payload.slice(0, 8));
        var time_diff = Date.now() - time_stamp;
        var msg = byteArrayToIntArray(payload.slice(10, length));
        var counter = msg[0];
        var user_id = msg[1];
        var game_flag = msg[2];
        var eeg_data_len = msg[3];
        var eeg_data1 = msg.slice(6, 6 + eeg_data_len);
        var eeg_data2 = msg.slice(6 + eeg_data_len, 6 + eeg_data_len + eeg_data_len);
        var eeg_data3 = msg.slice(6 + eeg_data_len + eeg_data_len, 6 + eeg_data_len + eeg_data_len + eeg_data_len);
        window.payload = payload;
        window.user_id = user_id;
        window.game_buf = game_buf;
        game_buf.push(game_flag);

        window.eeg_data1 = eeg_data1;
        window.eeg_data2 = eeg_data2;
        window.eeg_data3 = eeg_data3;

        window.eeg_data11 = eeg_data11;
        window.eeg_data12 = eeg_data12;
        window.eeg_data13 = eeg_data13;
        eeg_data11.push(...eeg_data1);
        eeg_data12.push(...eeg_data2);
        eeg_data13.push(...eeg_data3);

        console.log("Publish topic is " + message.destinationName);
        console.log("Bytes received in total: " + length);
        //console.log("Timestamp when the data was sent: " + time_stamp);
        //console.log("Timestamp when the data was received: " + Date.now());
        //console.log("Time difference in milliseconds: " + time_diff);
        //console.log(counter);
        console.log("User ID: " + user_id);
        console.log("Maze flag: " + game_flag);
        cnt++;
        arr.push(time_diff);
        var latency = arr[cnt] - arr[cnt - 1] - 3000;
        arr1.push(latency);
        //console.log(arr1);
        console.log("Latency: " + latency);
        console.log("Game Data Array: [" + game_buf.join(' ') + "]");
    }



    var totalPoints = 300;
    function getInitialData() {
        var res = [];
        for (var i = 0; i < totalPoints; ++i) {
            res.push([i, -1000]);
        }

        return res;
    }
    window.data11 = [];
    window.eeg_data11 = [];

    window.data12 = [];
    window.eeg_data12 = [];

    window.data13 = [];
    window.eeg_data13 = [];

    function getRealData1() {
        if (data11.length > 0)
            data11 = data11.slice(1);
        var j = 0;
        while (data11.length < eeg_data11.length) {
            data11.push(eeg_data11[j]);
            j++;
        }
        var res = [];
        for (var i = 0; i < data11.length; ++i) {
            res.push([i, data11[i]])
        }
        return res;

    }
    function getRealData2() {
        if (data12.length > 0)
            data12 = data12.slice(1);
        var j = 0;
        while (data12.length < eeg_data12.length) {
            data12.push(eeg_data12[j]);
            j++;
        }

        var res = [];
        for (var i = 0; i < data12.length; ++i) {
            res.push([i, data12[i]])
        }
        return res;

    }

    function getRealData3() {
        if (data13.length > 0)
            data13 = data13.slice(1);
        var j = 0;
        while (data13.length < eeg_data13.length) {
            data13.push(eeg_data13[j]);
            j++;
        }

        var res = [];
        for (var i = 0; i < data13.length; ++i) {
            res.push([i, data13[i]])
        }
        return res;

    }

    function update() {

        plot1.setData([getRealData1()]);
        plot1.draw();

        plot2.setData([getRealData2()]);
        plot2.draw();

        plot3.setData([getRealData3()]);
        plot3.draw();
        setTimeout(update, $('html').hasClass('mobile-device') ? 100 : 30);
    }
    var plot1 = $.plot('#flotRealTime1', [getInitialData()], {
        colors: ['#8CC9E8'],
        series: {
            lines: {
                show: true,
                fill: false,
                lineWidth: 1,
                fillColor: {
                    colors: [{
                        opacity: 0.45
                    }, {
                        opacity: 0.45
                    }]
                }
            },
            points: {
                show: false
            },
            shadowSize: 0
        },
        grid: {
            borderColor: 'rgba(0,0,0,0.1)',
            borderWidth: 1,
            labelMargin: 15,
            backgroundColor: 'transparent'
        },
        yaxis: {
            min: -250,
            max: 250,
            color: 'rgba(0,0,0,0.1)'
        },
        xaxis: {
            show: false
        }
    });

    var plot2 = $.plot('#flotRealTime2', [getInitialData()], {
        colors: ['#8CC9E8'],
        series: {
            lines: {
                show: true,
                fill: false,
                lineWidth: 1,
                fillColor: {
                    colors: [{
                        opacity: 0.45
                    }, {
                        opacity: 0.45
                    }]
                }
            },
            points: {
                show: false
            },
            shadowSize: 0
        },
        grid: {
            borderColor: 'rgba(0,0,0,0.1)',
            borderWidth: 1,
            labelMargin: 15,
            backgroundColor: 'transparent'
        },
        yaxis: {
            min: -100,
            max: 400,
            color: 'rgba(0,0,0,0.1)'
        },
        xaxis: {
            show: false
        }
    });

    var plot3 = $.plot('#flotRealTime3', [getInitialData()], {
        colors: ['#8CC9E8'],
        series: {
            lines: {
                show: true,
                fill: false,
                lineWidth: 1,
                fillColor: {
                    colors: [{
                        opacity: 0.45
                    }, {
                        opacity: 0.45
                    }]
                }
            },
            points: {
                show: false
            },
            shadowSize: 0
        },
        grid: {
            borderColor: 'rgba(0,0,0,0.1)',
            borderWidth: 1,
            labelMargin: 15,
            backgroundColor: 'transparent'
        },
        yaxis: {
            min: 100,
            max: 600,
            color: 'rgba(0,0,0,0.1)'
        },
        xaxis: {
            show: false
        }
    });

    var container = document.querySelector("#unity-container");
    var canvas = document.querySelector("#unity-canvas");
    var loadingBar = document.querySelector("#unity-loading-bar");
    var progressBarFull = document.querySelector("#unity-progress-bar-full");
    var fullscreenButton = document.querySelector("#unity-fullscreen-button");
    var warningBanner = document.querySelector("#unity-warning");

    // Shows a temporary message banner/ribbon for a few seconds, or
    // a permanent error message on top of the canvas if type=='error'.
    // If type=='warning', a yellow highlight color is used.
    // Modify or remove this function to customize the visually presented
    // way that non-critical warnings and error messages are presented to the
    // user.
    function unityShowBanner(msg, type) {
        function updateBannerVisibility() {
            warningBanner.style.display = warningBanner.children.length ? 'block' : 'none';
        }
        var div = document.createElement('div');
        div.innerHTML = msg;
        warningBanner.appendChild(div);
        if (type == 'error') div.style = 'background: red; padding: 10px;';
        else {
            if (type == 'warning') div.style = 'background: yellow; padding: 10px;';
            setTimeout(function () {
                warningBanner.removeChild(div);
                updateBannerVisibility();
            }, 5000);
        }
        updateBannerVisibility();
    }

    var buildUrl = "Build";
    var loaderUrl = buildUrl + "/Builds.loader.js";
    var config = {
        dataUrl: buildUrl + "/Builds.data",
        frameworkUrl: buildUrl + "/Builds.framework.js",
        codeUrl: buildUrl + "/Builds.wasm",
        streamingAssetsUrl: "StreamingAssets",
        companyName: "DefaultCompany",
        productName: "francescaaaaaa",
        productVersion: "0.1",
        showBanner: unityShowBanner,
    };

    // By default Unity keeps WebGL canvas render target size matched with
    // the DOM size of the canvas element (scaled by window.devicePixelRatio)
    // Set this to false if you want to decouple this synchronization from
    // happening inside the engine, and you would instead like to size up
    // the canvas DOM size and WebGL render target sizes yourself.
    // config.matchWebGLToCanvasSize = false;

    if (/iPhone|iPad|iPod|Android/i.test(navigator.userAgent)) {
        // Mobile device style: fill the whole browser client area with the game canvas:

        var meta = document.createElement('meta');
        meta.name = 'viewport';
        meta.content = 'width=device-width, height=device-height, initial-scale=1.0, user-scalable=no, shrink-to-fit=yes';
        document.getElementsByTagName('head')[0].appendChild(meta);
        container.className = "unity-mobile";

        // To lower canvas resolution on mobile devices to gain some
        // performance, uncomment the following line:
        // config.devicePixelRatio = 1;

        canvas.style.width = window.innerWidth + 'px';
        canvas.style.height = window.innerHeight + 'px';

        unityShowBanner('WebGL builds are not supported on mobile devices.');
    } else {
        // Desktop style: Render the game canvas in a window that can be maximized to fullscreen:

        canvas.style.width = "960px";
        canvas.style.height = "600px";
    }

    loadingBar.style.display = "block";

    var script = document.createElement("script");
    script.src = loaderUrl;
    script.onload = () => {
        createUnityInstance(canvas, config, (progress) => {
            progressBarFull.style.width = 100 * progress + "%";
        }).then((unityInstance) => {
            loadingBar.style.display = "none";
            fullscreenButton.onclick = () => {
                unityInstance.SetFullscreen(1);
            };
        }).catch((message) => {
            alert(message);
        });
    };
    document.body.appendChild(script);

}).apply(this, [jQuery]);

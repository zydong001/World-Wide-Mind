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
        'a1y3vnxyufqzra-ats.iot.eu-west-2.amazonaws.com', // Require 'lowercamelcase'!!
        'AKIAQCSSQ66HI5GBRF3F',
        'ncUwImn89sBSkv9XWdXPKo+rAcvtX183mkUcOSZ2');
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
        console.log("Game Data Array: [" + game_buf.join(' ') +"]");
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
        setTimeout(update, $('html').hasClass('mobile-device') ? 100: 30);
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
}).apply(this, [jQuery]);

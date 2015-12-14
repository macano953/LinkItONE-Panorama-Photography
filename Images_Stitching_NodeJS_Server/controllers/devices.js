exports.getImages = function (req, res) {
	connection.query('SELECT lat, lon, drone, unix_timestamp(im_time) as unix_time FROM list', function (error, rows, fields) {
		if (error) {
			throw error;
		} else {
			if (rows.length == 0)
				res.status(404).jsonp({
					msg : "Error",
					info : "No registers found"
				});
			else
				res.status(200).jsonp(rows);
		}
	});
}

exports.getTimestamp = function (req, res) {
	var timestamp = Date.now() / 1000 | 0;
	console.log('GET timestamp: ' + timestamp);
	res.status(200).jsonp({
		timestamp : timestamp
	});
}

exports.uploadLinkit = function (req, res) {
	image_treatment(req, res);
}

exports.upload = function (req, res) {
	var image_front = require('fs').readFileSync(__dirname + '/images/front.jpg');
	var image_back = require('fs').readFileSync(__dirname + '/images/back.jpg');
	var image_left = require('fs').readFileSync(__dirname + '/images/left.jpg');
	var image_right = require('fs').readFileSync(__dirname + '/images/right.jpg');
	var query = "INSERT INTO list SET ?",
	values = {
		lat : 52.2208361,
		lon : -0.0741336,
		drone : 1,
		image_front : image_front,
		image_back : image_back,
		image_left : image_left,
		image_right : image_right
	};
	connection.query(query, values, function (error, rows, fields) {
		if (error) {
			throw error;
		} else {
			res.status(200).jsonp('OK');
		}
	});
}

exports.findImages = function (req, res) {
	connection.query('SELECT image_front, image_right, image_back, image_left FROM list WHERE im_time = FROM_UNIXTIME(?) AND drone = ?', [req.params.id, req.params.drone], function (error, rows, fields) {
		if (error) {
			throw error;
		} else {
			if (rows.length == 0)
				res.status(404).jsonp({
					msg : "Error",
					info : "Ningún registro almacenado"
				});
			else {
				var bufferBase64_1 = new Buffer(rows[0].image_front).toString('base64');
				var bufferBase64_2 = new Buffer(rows[0].image_right).toString('base64');
				var bufferBase64_3 = new Buffer(rows[0].image_back).toString('base64');
				var bufferBase64_4 = new Buffer(rows[0].image_left).toString('base64');
				res.status(200).jsonp({
					image_front : bufferBase64_1,
					image_right : bufferBase64_2,
					image_back : bufferBase64_3,
					image_left : bufferBase64_4
				});
			}
		}
	});
}

String.prototype.replaceAll = function (str1, str2, ignore) {
	return this.replace(new RegExp(str1.replace(/([\/\,\!\\\^\$\{\}\[\]\(\)\.\*\+\?\|\<\>\-\&])/g, "\\$&"), (ignore ? "gi" : "g")), (typeof(str2) == "string") ? str2.replace(/\$/g, "$$$$") : str2);
}

function image_treatment(req, res) {
	var fs = require('fs');
	var image;
	var error = "error";
	var success = "OK";
	console.log(req.body.timestamp + '_' + req.body.drone + ' is being treated...');
	if (req.body.image1 !== undefined) {
		image = req.body.image1;
		var cam = 'CAM1.JPG';
	} else if (req.body.image2 !== undefined) {
		image = req.body.image2;
		var cam = 'CAM2.JPG';
	} else if (req.body.image3 !== undefined) {
		image = req.body.image3;
		var cam = 'CAM3.JPG';
	} else if (req.body.image4 !== undefined) {
		image = req.body.image4;
		var cam = 'CAM4.JPG';
	} else
		console.log("Not recognised image");
	var image1 = image.replaceAll(" ", "+");
	var image_decoded = new Buffer(image1, 'base64');	
	image_decoded = image_decoded.slice(1, image_decoded.length);
	try {
		require('lwip').open(image_decoded, 'jpg', function (err, image) {
			if (err)
				res.status(200).jsonp({
					status : error
				});
			else {
				// define a batch of manipulations and save to disk as JPEG:
				image.batch()
				.rotate(180) // rotate 180degs clockwise (white fill)
				.writeFile(__dirname + '/images/' + req.body.timestamp.toString() + '_' + req.body.drone + '_' + cam, function (err) {
					var error = "error";
					var success = "OK";
					if (err) {
						res.status(200).jsonp({
							status : error
						});
						console.log(err);
					} else {
						res.status(200).jsonp({
							status : success
						});
						if (req.body.image4 !== undefined) {
							//Look for 4 images and generates a panorama view using Hugin command tool
							generate_panorama(req, res);
						}
					}
				});
			}
		});
	} catch (ex) {
		console.log('exception when opening lwip');
		res.status(200).jsonp({
			status : error
		});
	}
}
function generate_panorama(req, res) {
	var panoramit = require('panoramit'),
	path = require('path');
	var genImagePath = path.join(__dirname + '/images/' + req.body.timestamp.toString() + '_' + req.body.drone + '_' + 'panorama.jpg');
	var back = path.join(__dirname + '/images/' + req.body.timestamp.toString() + '_' + req.body.drone + '_' + 'CAM4.JPG');
	var front = path.join(__dirname + '/images/' + req.body.timestamp.toString() + '_' + req.body.drone + '_' + 'CAM2.JPG');
	var left = path.join(__dirname + '/images/' + req.body.timestamp.toString() + '_' + req.body.drone + '_' + 'CAM3.JPG');
	var right = path.join(__dirname + '/images/' + req.body.timestamp.toString() + '_' + req.body.drone + '_' + 'CAM1.JPG');

	panoramit.generate({
		inputPaths : [front, right, back, left],
		outputFile : genImagePath,
		tempDir : path.join(__dirname, 'temp'),
		debug : true // optional value in case you want to debug the individual panorama commands
	}, function (err, outputPath) {
		require('lwip').open(genImagePath, function (err, imagen) {
			if (err)
				console.log(err);
			var _cropOpt = {
				left : 100,
				top : 0,
				right : 2188,
				bottom : 800
			}; // extract the face from the pic

			var _scaleOpt = {
				width : 2048,
				height : 1024
			};
			imagen.batch()
			//.crop(_cropOpt.left, _cropOpt.top, _cropOpt.right, _cropOpt.bottom)
			.resize(_scaleOpt.width, _scaleOpt.height)
			.toBuffer('jpg', function (err, buffer) {
				if (err)
					console.log(err);
				else {					
					var query = "INSERT INTO list SET ?",
					values = {
						lat : req.body.lat,
						lon : req.body.lon, 
						drone : 1,
						image_front : buffer,
						image_back : buffer,
						image_left : buffer,
						image_right : buffer
					};
					connection.query(query, values, function (error, rows, fields) {
						if (error)
							console.log(error);
						else
							console.log("Successfully stored on DB");
					});
				}
			});
		});
	});
}

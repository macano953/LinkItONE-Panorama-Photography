/*
 * Function: getTimestamp
 * ----------------------------
 *   Returns the current timestamp to give clients a way to identify pictures before taking them if 
 *   they have not an internal clock.
 *   receives: 
 		- req (HTTP request object)
 		- res (HTTP respose object)
 *   returns: nothing 
 * ----------------------------
 */
exports.getTimestamp = function (req, res) {
	res.status(200).jsonp({
		timestamp : Date.now() / 1000 | 0
	});
}
/*
 * Function: uploadLinkit
 * ----------------------------
 *   Handles the incoming of new images. Images are processed (resized, cropped, etc) and saved 
 *   before stitching them. Once we have one image for each camera in a certain moment, an automatic
 *   process is triggered to stitch them together.
 *   receives: 
 		- req (HTTP request object)
 		- res (HTTP respose object)
 *   returns: nothing 
 * ----------------------------
 */
exports.uploadLinkit = function (req, res) {
	image_treatment(req, res);
}

/*
 * Function: image_treatment
 * ----------------------------
 *   Process every image that reach the server. 
 *   For this purpose we will make use of lwip node module (https://www.npmjs.com/package/lwip), an image 
 *   processor for Node.js.
 *   receives: 
 		- req (HTTP request object)
 		- res (HTTP respose object)
 *   returns: nothing 
 * ----------------------------
 */

function image_treatment(req, res) {
	var fs = require('fs');
	var image;
	var error = "error";
	var success = "OK";
	console.log('An image with timestamp '+ req.body.timestamp + ' is being treated...');
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
	} else{
		console.log("Not recognised image");
		var cam = 'not_registered_CAM.JPG';
	}
	var image_decoded = new Buffer(image.replaceAll(" ", "+"), 'base64');	
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
				.rotate(0) // rotate clockwise (white fill) if needed. 
				.writeFile(__dirname + '/images/' + req.body.timestamp.toString() +'_' + req.body.id + '_' + cam, function (err) {
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

/*
 * Function: generate_panorama
 * ----------------------------
 *   Gets 4 images as input and returns the panorama view stored on the directory pointed by *outputPath*.
 *   In order to leave the image perfectly prepared to be consumed by panoramaGL library, it is needed to
 *   resize it, crop it and store it as RAW data on the MySQL database deployed. 
 *   For this purpose we will make use of lwip node module (https://www.npmjs.com/package/lwip), an image 
 *   processor for Node.js.
 *   receives: 
 		- req (HTTP request object)
 		- res (HTTP respose object)
 *   returns: nothing 
 * ----------------------------
 */

function generate_panorama(req, res) {
	var panorama = require('panorama'),
	path = require('path');
	var genImagePath = path.join(__dirname + '/images/' + req.body.timestamp.toString() + '_' + req.body.id + '_' + 'panorama.jpg');
	var back = path.join(__dirname + '/images/' + req.body.timestamp.toString() + '_' + req.body.id + '_' + 'CAM4.JPG');
	var front = path.join(__dirname + '/images/' + req.body.timestamp.toString() + '_' + req.body.id + '_' + 'CAM2.JPG');
	var left = path.join(__dirname + '/images/' + req.body.timestamp.toString() + '_' + req.body.id + '_' + 'CAM3.JPG');
	var right = path.join(__dirname + '/images/' + req.body.timestamp.toString() + '_' + req.body.id + '_' + 'CAM1.JPG');

	panorama.generate({
		inputPaths : [front, right, back, left],
		outputFile : genImagePath,
		tempDir : path.join(__dirname, 'temp'),
		debug : true // optional value in case you want to debug the individual panorama commands
	}, function (err, outputPath) {
		require('lwip').open(genImagePath, function (err, image) {
			if (err)
				console.log(err);
			var _cropOpt = {
				left : 100,
				top : 0,
				right : 2188,
				bottom : 800
			}; 
			var _scaleOpt = {
				width : 2048,
				height : 1024
			};
			image.batch()
			.resize(_scaleOpt.width, _scaleOpt.height)
			.writeFile(outputPath, function (err) {
				if (err)
					console.log(err);
				else {					
					console.log('Panorama image successfully created!');
				}
			});
		});
	});
}

String.prototype.replaceAll = function (str1, str2, ignore) {
	return this.replace(new RegExp(str1.replace(/([\/\,\!\\\^\$\{\}\[\]\(\)\.\*\+\?\|\<\>\-\&])/g, "\\$&"), (ignore ? "gi" : "g")), (typeof(str2) == "string") ? str2.replace(/\$/g, "$$$$") : str2);
}
function labelImage(className, imageName) {
    xhttp = new XMLHttpRequest();
    xhttp.open("GET", `labelImage?className=${encodeURIComponent(className)}&imageName=${encodeURIComponent(imageName)}`);
    xhttp.send();
}

function listImages(dataset, onloadFun) {
    req = new XMLHttpRequest();
    req.responseType = 'json';
    req.onload = function () {
        onloadFun(req.response)
    }
    req.open("GET", 'cgi-bin?app=listImages&type=' + dataset, true);
    req.send();
}


function predictOnDataset(sourceDataset, sourceClass, onloadFun) {
    req = new XMLHttpRequest();
    req.responseType = 'json';
    req.onload = function () {
        onloadFun(req.response)
    }
    req.open("GET", 'cgi-bin?app=predictOnDataset&' +
        'sourceDataset=' + sourceDataset +
        '&sourceClass=' + sourceClass, true);
    req.send();
}

function relabelImage(sourceImage) {
    selectedClassObj = document.querySelector('input[name="class"]:checked');
    if (!selectedClassObj) {
        alert('must select a target class first');
        return;
    }
    targetClass = selectedClassObj.value;
    targetDivContainer = document.getElementById('div-for-images-with-color-' + targetClass);
    srcDiv = document.getElementById(sourceImage);
    targetDivContainer.appendChild(srcDiv);
    req = new XMLHttpRequest();
    req.open("GET", 'cgi-bin?app=relabelImage&sourceImage=' + sourceImage + '&targetClass=' + targetClass, true);
    req.send();
}

function makeImageLabelCallback(sourceImage) {
    return function () { relabelImage(sourceImage); };
}
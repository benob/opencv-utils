
function Split(type) {
    this.type = type;
    this.subshots = [];
    this.add = function(subshot) {
        this.subshots.push(subshot);
        $(this.view).find('ol').append($(subshot.view));
    }
    this.remove = function(subshot) {
        var index = this.subshots.indexOf(subshot);
        if(index >= 0) {
            this.subshots.splice(index, 1);
            $(this.view).find('ol > li:nth-child(' + (index + 1) + ')').remove();
            redraw();
        }
    }
    this.toString = function() {
        var output = {type: this.type, subshots: []};
        for(var i = 0; i < this.subshots.length; i++) {
            output.subshots.push(this.subshots[i].getRect());
        }
        return JSON.stringify(output);
    }
    this.drawOn = function(context) {
        $(this.view).find('span').text(this.toString());
        for(var i = 0; i < this.subshots.length; i++) {
            this.subshots[i].drawFrom(shot_image);
            this.subshots[i].drawOn(context);
            context.fillStyle = "#ff00ff";
            context.textAlign = "center";
            context.textBaseline = "middle";
            context.font = "20pt Arial";
            var rect = this.subshots[i].getRect();
            context.fillText("" + (i + 1), rect.x + rect.width / 2, rect.y + rect.height / 2);
        }
    }
    this.fromString = function(label) {
        this.type = label.type;
        for(var i = 0; i < label.subshots.length; i++) {
            var rect = label.subshots[i];
            var subshot = new SubShot([rect.x, rect.y]);
            subshot.update([rect.x + rect.width, rect.y + rect.height]);
            this.add(subshot);
        }
    }
    this.view = $("<div>").append($('<img>').attr('src', 'splits/' + type + '.png'))
        .append('<div class="templates"></div>')
        .append('<br>')
        .append($('<span>').text(this.toString()))
        .append($('<button>Save</button>').click(function() {
            templates.push(split.toString());
            localStorage['split-templates:' + split.type] = JSON.stringify(templates);
            split.drawTemplates();
        }))
        .append('<ol>');
    this.drawTemplates = function() {
        $(this.view).find('.templates').empty();
        if('split-templates:' + this.type in localStorage) {
            templates = JSON.parse(localStorage['split-templates:' + this.type]);
            for(var i = 0; i < templates.length; i++) {
                var template = JSON.parse(templates[i]);
                var canvas = $('<canvas>')[0];
                $(canvas).attr('template', templates[i]);
                canvas.width = 1024 / 8;
                canvas.height = 576 / 8;
                var context = canvas.getContext('2d');
                context.fillStyle = 'black';
                context.beginPath();
                context.rect(0, 0, canvas.width, canvas.height);
                context.fill();
                for(var j = 0; j < template.subshots.length; j++) {
                    context.fillStyle = "white";
                    var rect = template.subshots[j];
                    context.beginPath();
                    context.rect(rect.x / 4, rect.y / 4, rect.width / 4, rect.height / 4);
                    context.fill();
                }
                $(this.view).find('.templates').append($(canvas).click(function() {
                    $(split.view).find('ol').empty();
                    split.subshots = [];
                    split.fromString(JSON.parse($(this).attr('template')));
                    redraw();
                }));
            }
        }
    }
    this.drawTemplates();
}

function SubShot(origin) {
    var object = this;
    this.canvas = $('<canvas>')[0];
    this.view = $('<li></li>')
        .append($(this.canvas))
        .append($('<button>Remove</button>').click(function(event) {
            console.log(object);
            split.remove(object);
        }))
        ;
    this.origin = origin;
    this.target = origin;
    this.update = function(point) {
        this.target = point;
    }
    this.getRect = function() {
        var rect = {x: this.origin[0], y: this.origin[1], width: this.target[0] - this.origin[0], height: this.target[1] - this.origin[1]};
        if(this.target[0] < this.origin[0]) {
            rect.width = this.origin[0] - this.target[0];
            rect.x = this.target[0];
        }
        if(this.target[1] < this.origin[1]) {
            rect.height = this.origin[1] - this.target[1];
            rect.y = this.target[1];
        }
        return rect;
    }
    this.drawOn = function(context) {
        var rect = this.getRect();
        context.strokeStyle = '#ff00ff';
        context.fillStyle = 'rgba(0, 0, 0, 0.5)';
        context.beginPath();
        context.rect(rect.x, rect.y, rect.width, rect.height);
        context.fill();
        context.stroke();
    }
    this.drawFrom = function(image) {
        var rect = this.getRect();
        this.canvas.width = rect.width;
        this.canvas.height = rect.height;
        var context = this.canvas.getContext('2d');
        context.drawImage(image, rect.x, rect.y, rect.width, rect.height, 0, 0, rect.width, rect.height);
    }
}

function adjustCoords(mouseX, mouseY, radius) {
    var xmax = 0, xargmax = null;
    for(var i = mouseX - radius; i < mouseX + radius; i++) {
        if(i < 0 || i > rows.length - 1) continue;
        if(xargmax == null || rows[i] > xmax) {
            xmax = rows[i];
            xargmax = i;
        }
    }
    var ymax = 0; yargmax = null;
    for(var j = mouseY - radius; j < mouseY + radius; j++) {
        if(j < 0 || j > columns.length - 1) continue;
        if(yargmax == null || columns[j] > ymax) {
            ymax = columns[j];
            yargmax = j;
        }
    }
    return [xargmax, yargmax];
}

function redraw() {
    var ctx = $('body > canvas')[0].getContext('2d');
    ctx.drawImage(shot_image, 0, 0);
    split.drawOn(ctx);
}

columns = [];
rows = [];

split = null;
subshot = null;
shot_image = null;
templates = [];

$(function() {
    var src = 'images/BFMTV_BFMStory_2011-05-11_175900/BFMTV_BFMStory_2011-05-11_175900.MPG_0000018027.png';
    var split_type = '1-other';
    var argv = location.search.substring(1).split('&');
    for(var i = 0; i < argv.length; i++) {
        var pair = argv[i].split('=');
        console.log(pair);
        if(pair[0] == 'split') split_type = pair[1];
        else if(pair[0] == 'img') src = pair[1];
    }
    split = new Split(split_type);
    $('body').append(split.view);
    $('<img>').attr('src', src)
            .load(function() {
        shot_image = this;
        var grayscale = Filters.filterImage(Filters.grayscale, this);
        //var blured = Filters.convolute(grayscale, [1/9, 1/9, 1/9, 1/9, 1/9, 1/9, 1/9, 1/9, 1/9]);
        var vertical = Filters.convoluteFloat32(grayscale, [ -1, 0, 1, -2, 0, 2, -1, 0, 1 ]);
        var horizontal = Filters.convoluteFloat32(grayscale, [ -1, -2, -1, 0,  0,  0, 1,  2,  1 ]);

        for(var i = 0; i < this.width; i++) {
            var cumulative = 0;
            for(var j = 0; j < this.height; j++) {
                cumulative += Math.abs(vertical.data[4 * (i + j * this.width)]);
            }
            rows[i] = cumulative;
        }
        for(var j = 0; j < this.height; j++) {
            var cumulative = 0;
            for(var i = 0; i < this.width; i++) {
                cumulative += Math.abs(horizontal.data[4 * (i + j * this.width)]);
            }
            columns[j] = cumulative;
        }

        var c = $('<canvas>')[0];
        $('body').prepend($(c).css('float', 'right'));
        c.width = this.width;
        c.height = this.height;
        var ctx = c.getContext('2d');
        ctx.drawImage(this, 0, 0);
        var image = this;
        $(c).mousemove(function(e) {
            ctx.drawImage(image, 0, 0);
            var mouseX = e.pageX - this.offsetLeft;
            var mouseY = e.pageY - this.offsetTop;
            var coords = adjustCoords(mouseX, mouseY, 3);
            if(subshot) {
                subshot.update(coords);
                subshot.drawFrom(image);
            }
            split.drawOn(ctx);
            ctx.strokeStyle = 'red';
            ctx.beginPath();
            ctx.arc(coords[0], coords[1], 1, 0, 2 * Math.PI, false);
            ctx.stroke();
        }).mousedown(function(e) {
            origin = adjustCoords(e.pageX - this.offsetLeft, e.pageY - this.offsetTop, 3);
            subshot = new SubShot(origin);
            split.add(subshot);
        }).mouseup(function(e) {
            if(subshot != null) {
                subshot = null;
            }
        }).mouseenter(function() {
            cursor = true;
        }).mouseleave(function() {
            cursor = false;
        });

            });
});

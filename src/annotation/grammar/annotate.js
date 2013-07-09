if (typeof String.prototype.startsWith != 'function') {
    String.prototype.startsWith = function (str){
        return this.slice(0, str.length) == str;
    };
}

function makeLabel(split, labels) {
    var cardinality = parseInt(split);
    return split + '=' + labels.slice(0, cardinality).join('+');
}

function EventEmitter(type) {
    this.type = type;
    this.listeners = {};
    this.on = function(name, listener, callback) {
        if(!(name in this.listeners)) this.listeners[name] = [];
        this.listeners[name].push({
            listener: listener,
            callback: callback
        });
    }
    this.fire = function(name) {
        if(name in this.listeners) {
            for(var i in this.listeners[name]) {
                var item = this.listeners[name][i];
                item.callback(item.listener, this);
            }
        }
    }
}

function Label(type) {
    EventEmitter.call(this, type);
    this.type = type;
    this.labels = ['<none>', 'presentateur', 'journaliste', 'invite', 'journaliste(face)+invite(dos)', 'reportage', 'graphiques', 'autre'];
    this.dom = $('<select class="label"></select>');
    for(var i in this.labels) {
        this.dom.append($('<option></option>').attr('value', this.labels[i]).text(this.labels[i]));
    }
    $(this.dom).change(function() {
        this.object.fire('change');
    });
    this.get = function() {
        return $(this.dom).val();
    };
    this.set = function(label) {
        $(this.dom).val(label);
    };
    this.setDefault = function() {
        this.set(this.labels[0]);
    }
    this.setDefault();
    this.dom[0].object = this;
}

function ShotLabels(type) {
    EventEmitter.call(this, type);
    this.type = type;
    this.dom = $('<ul class="shotlabels"></ul>');
    this.labels = [];
    this.set = function(splitSize, labels) {
        while(this.labels.length < splitSize || (labels != undefined && this.labels.length < labels.length)) {
            var label = new Label();
            label.on('change', this, function(listener, target) {
                listener.fire('change');
            });
            this.labels.push(label);
            $(this.dom).append($('<li></li>').append(label.dom));
        }
        for(var i = 0; i < this.labels.length; i++) {
            if(i < splitSize) {
                $(this.labels[i].dom).parent().show();
            } else {
                $(this.labels[i].dom).parent().hide();
            }
        }
        if(labels != undefined) {
            for(i in this.labels) {
                this.labels[i].setDefault();
            }
            for(var i in labels) {
                this.labels[i].set(labels[i]);
            }
        }
    };
    this.setDefault = function() {
        for(var i in this.labels) {
            this.labels[i].setDefault();
            $(this.labels[i].dom).parent().hide();
        }
    }
    this.get = function(cardinality) {
        var list = [];
        if(cardinality != undefined) {
            for(var i = 0; i < cardinality; i++) list.push(this.labels[i].get());
        } else {
            for(var i in this.labels) {
                list.push(this.labels[i].get());
            }
        }
        return list;
    };
    this.dom[0].object = this;
}

function SplitSelector(type) {
    EventEmitter.call(this, type);
    this.type = type;
    this.split_types = ['1-full', '2-big-left', '2-horizontal', '3-big-left', '3-even', 
        '4-big-right', '2-big-right', '2-vertical', '3-big-right', '4-big-left', '4-even', '1-other'];
    this.splits = {};
    this.dom = $('<div class="splitselector"></div>');
    for(var i in this.split_types) {
        var split = $('<img>').attr('src', 'splits/' + this.split_types[i] + '.png')
            .attr('name', this.split_types[i])
            .attr('cardinality', parseInt(this.split_types[i]));
        split.click(function() {
            $(this.parentNode).find('img').removeClass('selected');
            $(this).addClass('selected');
            this.parentNode.object.fire('change');
        });
        $(this.dom).append(split);
        this.splits[this.split_types[i]] = split;
    }
    this.set = function(name) {
        if(name in this.splits) {
            $(this.dom).find('img').removeClass('selected');
            $(this.splits[name]).addClass('selected');
        }
    };
    this.setDefault = function() {
        this.set(this.split_types[0]);
    }
    this.get = function() {
        var selected = $(this.dom).find('.selected');
        if(selected.length > 0) return selected.attr('name');
        return null;
    }
    this.cardinality = function() {
        return parseInt(this.get());
    }
    this.dom[0].object = this;
}

function MostSimilar(type) {
    EventEmitter.call(this, type);
    this.type = type;
    this.byLabel = {};
    this.dom = $('<div class="mostsimilar"></div>');
    this.set = function(shot) {
        var video = shot.split('.')[0];
        var min = {};
        var argmin = {};
        for(var id in localStorage) {
            if(id.startsWith('percol:' + video)) {
                var annotation = JSON.parse(localStorage[id]);
                var label = makeLabel(annotation.split, annotation.labels);
                if(annotation.name != shot) {
                    var score = sim[shotIndex[shot].id][shotIndex[annotation.name].id];
                    if(!(label in min) || min[label] > score) {
                        min[label] = score;
                        argmin[label] = annotation.name;
                    }
                }
            }
        }
        var scored = [];
        for(var label in min) {
            scored.push({score: min[label], label: label, shot: argmin[label]});
        }
        scored.sort(function(a, b) {
            return a.score - b.score;
        });
        $(this.dom).empty();
        for(var i = 0; i < scored.length && i < 4; i++) {
            var target_shot = scored[i].shot;
            $(this.dom).append($('<img>')
                    .attr('title', scored[i].label + '\ndistance: ' + scored[i].score + '\nshot: ' + scored[i].shot)
                    .attr('src', shotIndex[scored[i].shot].image)
                    .attr('width', 1024 / 6)
                    .attr('height', 576 / 6).click(function() {
                        //load(target_shot);
                        //save(shot);
                    }));
        }
        console.log(scored);
    }
}

function Annotator(type) {
    EventEmitter.call(this, type);
    this.type = type;
    this.splitSelector = new SplitSelector();
    this.shotLabels = new ShotLabels();
    this.mostSimilar = new MostSimilar();
    this.shotName = null;
    
    this.dom = $('<div class="annotator"></div>')
        .append($('<div class="shotdiv">Shot: <span class="shotname"></span></div>'))
        .append($('<div class="labeldiv">Label: <span class="textlabel"></span></div>'))
        .append(this.splitSelector.dom)
        .append(this.shotLabels.dom)
        .append(this.mostSimilar.dom);

    this.save = function (shot) {
        var annotation = {
            name: shot,
            split: this.splitSelector.get(),
            labels: this.shotLabels.get()
        };
        localStorage['percol:' + shot] = JSON.stringify(annotation);
    };
    this.load = function(shot) {
        if('percol:' + shot in localStorage) {
            var annotation = JSON.parse(localStorage['percol:' + shot]);
            this.splitSelector.set(annotation.split);
            this.shotLabels.set(this.splitSelector.cardinality(), annotation.labels);
        } else {
            this.splitSelector.setDefault();
            this.shotLabels.setDefault();
        }
        this.fire('change');
    };
    this.set = function(shot) {
        this.shotName = shot;
        this.mostSimilar.set(shot);
        this.load(shot);
    };
    this.get = function() {
        return null;
    };
    this.splitSelector.on('change', this, function(listener, target) {
        listener.save(listener.shotName);
        listener.fire('change');
    });
    this.shotLabels.on('change', this, function(listener, target) {
        listener.save(listener.shotName);
        listener.fire('change');
    });
    this.on('change', this, function(listener, target) {
        listener.shotLabels.set(listener.splitSelector.cardinality());
        $(listener.dom).find('.shotname').text(listener.shotName);
        $(listener.dom).find('.textlabel').text(makeLabel(listener.splitSelector.get(), listener.shotLabels.get()));
    });
    this.dom[0].object = this;
}

var shotIndex = {};
var images = [];
var sim = [];

function basename(path) {
    var tokens = path.split('/');
    return tokens[tokens.length - 1];
}

$(function() {
    var annotator = new Annotator();
    $('#by-shot').append(annotator.dom);

    // populate videos
    $('#show').empty();
    for(var i in videos) {
        $('#show').append($("<option></option>").attr("value", videos[i]).text(videos[i])); 
    }
    $('#show').change(function() {
        var show = $(this).val();
        shotIndex = {};
        callback = function() {
            $('#shots').empty();
            for(var i in images) {
                var image = $('<img class="shot">');
                image.click(function(event) {
                    $('.shot').removeClass('selected');
                    $(this).addClass('selected');
                    annotator.set(basename(this.name));
                });
                $(image).attr('src', datadir + '/' + images[i])
                    .attr('name', images[i]);
                $('#shots').append(image);
                shotIndex[basename(images[i])] = {id: i, image: datadir + '/' + images[i]};
            }
            console.log(show);
        };
        $('#shots').empty();
        $(document.head).append($('<script lang="javascript">').attr('src', 'data/' + show + '.js'));
    });
    $('#show').change();
});

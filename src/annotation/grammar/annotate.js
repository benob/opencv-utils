if (typeof String.prototype.startsWith != 'function') {
    String.prototype.startsWith = function (str){
        return this.slice(0, str.length) == str;
    };
}

var Config = {
    labels: ['presentateur', 'journaliste', 'invite', 'journaliste(face)+invite(dos)', 'reportage', 'graphiques', 'autre'],
    split_types: ['1-full', '2-big-left', '2-horizontal', '3-big-left', '3-even', '4-big-right', '2-big-right', '2-vertical', '3-big-right', '4-big-left', '4-even', '1-other'],
};

function makeLabel(split, labels) {
    var cardinality = parseInt(split);
    return split + '=' + labels.slice(0, cardinality).join('+');
}

function exportAnnotation() {
    var result = [];
    for(var id in localStorage) {
        if(id.startsWith('percol:')) {
            var annotation = JSON.parse(localStorage[id]);
            result.push(annotation);
        }
    }
    return JSON.stringify(result);
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

function BatchLabeler(type) {
    EventEmitter.call(this, type);
    this.type = type;
    this.dom = $('<div></div>');
    this.labelList = $('<select></select>').addClass('label-list');
    this.labeledShots = $('<div>').addClass('labeled-shots');
    this.unlabeledShots = $('<div>').addClass('unlabeled-shots');
    this.applyButton = $('<button>Apply</button>').addClass('apply');
    $(this.dom)
        .append(this.labeledShots)
        .append(this.labelList)
        .append(this.applyButton)
        .append('<hr>')
        .append(this.unlabeledShots);
    this.update = function() {
        var seenLabels = {};
        for(var id in localStorage) {
            if(id.startsWith('percol:')) {
                var annotation = JSON.parse(localStorage[id]);
                var label = annotation.label;
                if(label == null) label = makeLabel(annotation.split, annotation.labels);
                if(!(label in seenLabels)) {
                    seenLabels[label] = [];
                }
                seenLabels[label].push(annotation.name);
            }
        }
        $(this.labelList).empty();
        for(var i in seenLabels) {
            $(this.labelList).append($('<option></option>').attr('value', i).text(i));
        }
        $(this.labelList).change(function() {
            this.object.set($(this).val());
        });
        $(this.labelList).find('img:nth-child(2)').prop('selected',true);
        this.set($(this.labelList).val());

        $(this.applyButton).click(function() {
            var shot = ($(this).parent()[0].object.labeledShots).find('.selected').attr('name');
            $($(this).parent()[0].object.unlabeledShots).find('img.selected').each(function() {
                annotator.copy(shot, $(this).attr('name'));
            });
            $(this).parent()[0].object.set($($(this).parent()[0].object.labelList).val());
        });
    }
    this.getAnnotated = function(label) {
        var annotated = {};
        // list annotated shots with that label
        for(var id in localStorage) {
            if(id.startsWith('percol:')) {
                var annotation = JSON.parse(localStorage[id]);
                if(annotation.label == label 
                        || (annotation.label == undefined 
                            && makeLabel(annotation.split, annotation.labels) == label)) {
                                annotated[annotation.name] = 1;
                            }
            }
        }
        return annotated;
    }

    this.set = function(label) {
        var annotated = this.getAnnotated(label);
        $(this.labeledShots).empty();
        $(this.unlabeledShots).empty();
        for(var i in annotated) {
            var image = datadir + '/' + i.split('.')[0] + '/' + i + '.192';
            $(this.labeledShots).append($('<img>')
                    .attr('src', image)
                    .attr('name', i)
                    .addClass('thumbnail')
                    .click(function() {
                        $(this).parent().find('.selected').removeClass('selected');
                        $(this).addClass('selected');
                        var shot = $(this).attr('name');
                        $(this).parent()[0].object.showUnannotated(shot, $($(this).parent()[0].object.labelList).val());
                    }));
        }
        $(this.labeledShots).find(':first-child').click();
    };
    this.showUnannotated = function(shot, label) {
        var annotated = this.getAnnotated(label);
        var unannotated = [];
        for(var name in shotIndex) {
            if(!(name in annotated)) {
                unannotated.push({name: name, score:sim[shotIndex[shot].id][shotIndex[name].id]});
            }
        }
        unannotated.sort(function(a, b) {
            return a.score - b.score;
        });
        $(this.unlabeledShots).empty();
        for(var i in unannotated) {
            var name = unannotated[i].name;
            $(this.unlabeledShots).append($('<img>')
                    .attr('src', shotIndex[name].image + '.192')
                    .attr('name', name)
                    .attr('title', unannotated[i].score)
                    .addClass('thumbnail')
                    .click(function() {
                        $(this).toggleClass('selected');
                    }));
        }
    }
    this.labelList[0].object = this;
    this.unlabeledShots[0].object = this;
    this.labeledShots[0].object = this;
    this.dom[0].object = this;
    this.update();
}

function Label(type) {
    EventEmitter.call(this, type);
    this.type = type;
    this.dom = $('<select></select>').addClass('label');
    for(var i in Config.labels) {
        this.dom.append($('<option></option>').attr('value', Config.labels[i]).text(Config.labels[i]));
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
        this.set(Config.labels[0]);
    }
    this.setDefault();
    this.dom[0].object = this;
}

function ShotLabels(type) {
    EventEmitter.call(this, type);
    this.type = type;
    this.dom = $('<ol></ol>').addClass('shotLabels');
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
    this.splits = {};
    this.dom = $('<div></div>').addClass('splitSelector');
    for(var i in Config.split_types) {
        var split = $('<img>').attr('src', 'splits/' + Config.split_types[i] + '.png')
            .attr('name', Config.split_types[i])
            .attr('cardinality', parseInt(Config.split_types[i]));
        split.click(function() {
            $(this.parentNode).find('img').removeClass('selected');
            $(this).addClass('selected');
            this.parentNode.object.fire('change');
        });
        $(this.dom).append(split);
        this.splits[Config.split_types[i]] = split;
    }
    this.set = function(name) {
        if(name in this.splits) {
            $(this.dom).find('img').removeClass('selected');
            $(this.splits[name]).addClass('selected');
        }
    };
    this.setDefault = function() {
        this.set(Config.split_types[0]);
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
    this.dom = $('<div></div>').addClass('mostsimilar');
    this.showLabel = function(label) {
        $(this.dom).find('img').removeClass('selected');
        $(this.dom).find('img').each(function(index, element) {
            if($(element).attr('label') == label) $(element).addClass('selected');
        });
    };
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
                    .addClass('thumbnail')
                    .attr('title', scored[i].label + '\ndistance: ' + scored[i].score + '\nshot: ' + scored[i].shot)
                    .attr('src', shotIndex[scored[i].shot].image)
                    .attr('target_shot', target_shot)
                    .attr('label', scored[i].label)
                    .attr('shot', shot).click(function() {
                        annotator.copy($(this).attr('target_shot'), $(this).attr('shot'));
                        annotator.load($(this).attr('shot'));
                        $(this).parent().find('img').removeClass('selected');
                        $(this).addClass('selected');
                    }));
        }
    };
}

function Annotator(type) {
    EventEmitter.call(this, type);
    this.type = type;
    this.splitSelector = new SplitSelector();
    this.shotLabels = new ShotLabels();
    this.mostSimilar = new MostSimilar();
    this.batchLabeler = new BatchLabeler();
    this.shotName = null;
    
    this.dom = $('<div class="annotator"></div>')
        .append($('<div class="shotdiv">Shot: <span class="shotname"></span></div>'))
        .append($('<div class="labeldiv">Label: <span class="textlabel"></span></div>'))
        .append(this.splitSelector.dom)
        .append(this.shotLabels.dom)
        .append(this.mostSimilar.dom);

    this.save = function (shot) {
        if(shot in shotIndex) {
            var annotation = {
                name: shot,
                split: this.splitSelector.get(),
                labels: this.shotLabels.get(),
                label: makeLabel(this.splitSelector.get(), this.shotLabels.get()),
                annotator: $('#annotator').val(),
                date: new Date(),
            };
            localStorage['percol:' + shot] = JSON.stringify(annotation);
            $('img.shot[name="' + shot + '"]').addClass('annotated');
        }
    };
    this.copy = function(source, target) {
        if('percol:' + source in localStorage) {
            var annotation = JSON.parse(localStorage['percol:' + source]);
            annotation.name = target;
            annotation.annotator = $('#annotator').val();
            annotation.date = new Date();
            localStorage['percol:' + target] = JSON.stringify(annotation);
            $('img.shot[name="' + target + '"]').addClass('annotated');
        } else {
            console.log('no annotation available for', source);
        }
    }

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
        return makeLabel(this.splitSelector.get(), this.shotLabels.get());
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
        $(listener.dom).find('.textlabel').text(listener.get());
        listener.mostSimilar.showLabel(listener.get());
    });
    this.dom[0].object = this;
}

var shotIndex = {};
var images = [];
var sim = [];
var annotator = null;

function basename(path) {
    var tokens = path.split('/');
    return tokens[tokens.length - 1];
}

$(function() {
    annotator = new Annotator();
    $('#by-shot').append(annotator.dom);

    $("input[name='view']").change(function() {
        if($(this).val() == 'by-shot') {
            $('#by-shot').show();
            $('#by-label').hide();
        } else if($(this).val() == 'by-label') {
            $('#by-shot').hide();
            $('#by-label').show();
            annotator.batchLabeler.update();
        }
    });

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
            $('#by-label-shots').empty();
            for(var i in images) {
                var name = basename(images[i]);
                shotIndex[name] = {id: i, image: datadir + '/' + images[i]};
                // by shot
                var image = $('<img class="shot">');
                console.log('percol:' + name);
                if('percol:' + name in localStorage) {
                    $(image).addClass('annotated');
                }
                image.click(function(event) {
                    $('.shot').removeClass('selected');
                    $(this).addClass('selected');
                    annotator.set($(this).attr('name'));
                });
                $(image).attr('src', datadir + '/' + images[i])
                    .attr('name', name);
                $('#shots').append(image);

                $('#by-label').empty().append(annotator.batchLabeler.dom);
            }
            annotator.batchLabeler.update();
        };
        $('#shots').empty();
        $(document.head).append($('<script lang="javascript">').attr('src', 'data/' + show + '.js'));
    });
    $('#show').change();

    $('#export').click(function() {
        window.open('data:application/json;charset=utf-8,' + exportAnnotation());
    });

    // save annotator name
    if('annotatorName' in localStorage) {
        $('#annotator').val(localStorage['annotatorName']);
    }
    $('#annotator').change(function() {
        localStorage['annotatorName'] = $(this).val();
    });

    $('#by-shot').hide();
    $('#by-label').show();
});

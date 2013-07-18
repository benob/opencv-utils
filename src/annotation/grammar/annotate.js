var video = null;

if (typeof String.prototype.startsWith != 'function') {
    String.prototype.startsWith = function (str){
        return this.slice(0, str.length) == str;
    };
}

var Config = {
    // forbidden characters: '=', ':', ',' and '|'
    roles: ['Presentateur', 'Journaliste', 'Invité/inteviewé', 'Personnes d\'intêret', 'Autre'],
    poses: ['Face', 'Profil ->', '<- Profil', 'Dos', 'Autre'],
    locations: ['Centre', 'Gauche', 'Droite'],
    subshots: ['Plateau', 'Plateau + reportage incrusté', 'Reportage', 'Reportage + foule', 'Graphique', 'Jingle', 'Autre'],
    splits: ['1-full', '2-big-left', '2-horizontal', '3-big-left', '3-even', '4-big-right', '2-big-right', '2-vertical', '3-big-right', '4-big-left', '4-even', '1-other'],
};

function Annotation(label) {
    if(label == undefined) {
        this.split = Config.splits[0];
        var cardinality = parseInt(this.split);
        this.subshots = [];
        for(var i = 0; i < cardinality; i++) {
            this.subshots.push({type: Config.subshots[0], persons: [],});
        }
    } else if(typeof(label) == typeof('')) {
        var parts = label.split('=');
        this.split = parts[0];
        parts = parts[1].split('|');
        this.subshots = [];
        for(var i in parts) {
            var tokens = parts[i].split(':');
            var subshot = { type: tokens[0], persons: [] };
            tokens = tokens[1].split(',');
            for(var j in tokens) {
                var info = tokens[j].split('+');
                subshot.persons.push({
                    role: info[0],
                    pose: info[1],
                    location: info[2],
                });
            }
            this.subshots.push(subshot);
        }
    } else {
        this.split = label.split;
        this.subshots = label.subshots;
    }
    this.toString = function() {
        var subshots = [];
        for(var i in this.subshots) {
            var persons = [];
            for(var j in this.subshots[i].persons) {
                var person = this.subshots[i].persons[j];
                persons.push(person.role + '+' + person.pose + '+' + person.location);
            }
            subshots.push(this.subshots[i].type + ':' + persons.join(','));
        }
        var result = (this.split + '=' + subshots.join('|'));
        return result;
    }
}

// DEPRECATED
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
            if(id.startsWith('percol:' + video)) {
                var annotation = JSON.parse(localStorage[id]);
                var label = new Annotation(annotation).toString();
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
            if(id.startsWith('percol:' + video)) {
                var annotation = JSON.parse(localStorage[id]);
                if(new Annotation(annotation).toString() == label ) {
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
            var img = $('<img>')
                    .attr('src', shotIndex[name].image + '.192')
                    .attr('name', name)
                    .attr('title', 'distance: ' + unannotated[i].score + '\nshot: ' + unannotated[i].name)
                    .addClass('thumbnail')
                    .click(function() {
                        $(this).toggleClass('selected');
                    });
            if('percol:' + unannotated[i].name in localStorage) {
                var shotLabel = new Annotation(JSON.parse(localStorage['percol:' + unannotated[i].name])).toString();
                $(img).addClass('hasLabel')
                    .attr('title', shotLabel + '\ndistance: ' + unannotated[i].score + '\nshot: ' + unannotated[i].name);
            }
            $(this.unlabeledShots).append(img);
        }
    }
    this.labelList[0].object = this;
    this.unlabeledShots[0].object = this;
    this.labeledShots[0].object = this;
    this.dom[0].object = this;
    this.update();
}

function PersonAnnotation(type) {
    EventEmitter.call(this, type);
    this.type = type;
    this.dom = $('<div></div>').addClass('person');
    this.role = $('<select></select>').addClass('role');
    for(var i in Config.roles) {
        this.role.append($('<option></option>').attr('value', Config.roles[i]).text(Config.roles[i]));
    }
    $(this.dom).append(this.role);
    this.pose = $('<select></select>').addClass('pose');
    for(var i in Config.poses) {
        this.pose.append($('<option></option>').attr('value', Config.poses[i]).text(Config.poses[i]));
    }
    $(this.dom).append(this.pose);
    this.location = $('<select></select>').addClass('location');
    for(var i in Config.locations) {
        this.location.append($('<option></option>').attr('value', Config.locations[i]).text(Config.locations[i]));
    }
    $(this.dom).append(this.location);
    $(this.dom).find('select').change(function() {
        $(this).parent()[0].object.fire('change');
    });
    this.get = function() {
        return {role: $(this.role).val(), pose: $(this.pose).val(), location: $(this.location).val()};
    };
    this.set = function(person) {
        $(this.role).val(person.role);
        $(this.pose).val(person.pose);
        $(this.location).val(person.location);
    };
    this.setDefault = function() {
        this.set({role: Config.roles[0], pose: Config.poses[0], location: Config.locations[0]});
    }
    this.setDefault();
    this.dom[0].object = this;
}

function SubshotAnnotation(type) {
    EventEmitter.call(this, type);
    this.type = type;
    this.dom = $('<div></div>').addClass('subshot');
    this.type = $('<select></select>').addClass('type');
    for(var i in Config.subshots) {
        this.type.append($('<option></option>').attr('value', Config.subshots[i]).text(Config.subshots[i]));
    };
    $(this.type).val(Config.subshots[0]);
    $(this.type).change(function() {
        $(this).parent()[0].object.fire('change');
    });
    $(this.dom).append('Type: ').append(this.type);
    this.numPersons = $('<select></select>').addClass('numPersons');
    for(var i = 0; i < 10; i++) {
        this.numPersons.append($('<option></option>').attr('value', i).text(i));
    }
    $(this.dom).append('Persons: ').append(this.numPersons);
    $(this.numPersons).val(0);
    $(this.numPersons).change(function() {
        var num = parseInt($(this).val());  
        $(this).parent().find('.personItem').hide();
        for(var i = 0; i < num; i++) {
            $($(this).parent()[0].object.persons[i].dom).parent().show();
        }
        $(this).parent()[0].object.fire('change');
    });
    var personList = $('<ol></ol>');
    this.persons = [];
    for(var i = 0; i < 10; i++) {
        var person = new PersonAnnotation();
        personList.append($('<li></li>').addClass('personItem').append(person.dom).hide());
        this.persons.push(person);
        person.on('change', this, function(listener, target) {
            listener.fire('change');
        });
    }
    this.setDefault = function() {
        this.set({type: Config.subshots[0], persons: []});
    };
    this.set = function(subshot) {
        $(this.type).val(subshot.type);
        $(this.numPersons).val(subshot.persons.length);
        $(this.dom).find('.personItem').hide();
        for(var i in subshot.persons) {
            this.persons[i].set(subshot.persons[i]);
            $(this.persons[i].dom).parent().show();
        }
    };
    this.get = function() {
        var subshot = {type: $(this.type).val(), persons:[] };
        var num = parseInt($(this.numPersons).val());
        for(var i = 0; i < num; i++) {
            subshot.persons.push(this.persons[i].get());
        }
        return subshot;
    };
    $(this.dom).append(personList);
    this.dom[0].object = this;
}

function ShotLabels(type) {
    EventEmitter.call(this, type);
    this.type = type;
    this.dom = $('<ol></ol>').addClass('shotLabels');
    this.subshots = [];
    this.cardinality = 0;
    for(var i = 0; i < 10; i++) {
        var subshot = new SubshotAnnotation();
        subshot.on('change', this, function(listener, target) {
            listener.fire('change');
        });
        this.subshots.push(subshot);
        $(this.dom).append($('<li></li>').addClass('subshotItem').append(subshot.dom).hide());
    }
    this.set = function(subshots) {
        this.cardinality = subshots.length;
        $(this.dom).find('.subshotItem').hide();
        for(var i in subshots) {
            this.subshots[i].set(subshots[i]);
            $(this.subshots[i].dom).parent().show();
        }
    };
    this.setDefault = function(cardinality) {
        this.cardinality = cardinality || 0;
        $(this.dom).find('.subshotItem').hide();
        for(var i = 0; i < this.cardinality; i++) {
            this.subshots[i].setDefault();
            $(this.subshots[i].dom).parent().show();
        }
    }
    this.get = function() {
        var subshots = [];
        for(var i = 0; i < this.cardinality; i++) {
            subshots.push(this.subshots[i].get());
        }
        return subshots;
    };
    this.dom[0].object = this;
}

function SplitSelector(type) {
    EventEmitter.call(this, type);
    this.type = type;
    this.splits = {};
    this.dom = $('<div></div>').addClass('splitSelector');
    for(var i in Config.splits) {
        var split = $('<img>').attr('src', 'splits/' + Config.splits[i] + '.png')
            .attr('name', Config.splits[i])
            .attr('cardinality', parseInt(Config.splits[i]));
        split.click(function() {
            $(this.parentNode).find('img').removeClass('selected');
            $(this).addClass('selected');
            $(this).parent()[0].object.fire('change');
        });
        $(this.dom).append(split);
        this.splits[Config.splits[i]] = split;
    }
    this.set = function(name) {
        if(name in this.splits) {
            $(this.dom).find('img').removeClass('selected');
            $(this.splits[name]).addClass('selected');
        }
    };
    this.setDefault = function() {
        this.set(Config.splits[0]);
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
    this.numSimilar = 12;
    this.byLabel = {};
    this.dom = $('<div></div>').addClass('mostsimilar');
    this.showLabel = function(label) {
        $(this.dom).find('img').removeClass('selected');
        $(this.dom).find('img').each(function(index, element) {
            if($(element).attr('label') == label) $(element).addClass('selected');
        });
    };
    this.set = function(shot) {
        var min = {};
        var argmin = {};
        for(var id in localStorage) {
            if(id.startsWith('percol:' + video)) {
                var annotation = JSON.parse(localStorage[id]);
                var label = new Annotation(annotation).toString();
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
        for(var i = 0; i < scored.length && i < this.numSimilar; i++) {
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
                subshots: this.shotLabels.get(),
                label: this.get(),
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
            this.shotLabels.set(annotation.subshots);
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
        var annotation = new Annotation();
        annotation.split = this.splitSelector.get();
        annotation.subshots = this.shotLabels.get();
        return annotation;
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
        if(listener.splitSelector.cardinality() != listener.shotLabels.cardinality) {
            listener.shotLabels.setDefault(listener.splitSelector.cardinality());
        }
        $(listener.dom).find('.shotname').text(listener.shotName);
        $(listener.dom).find('.textlabel').text(listener.get().toString());
        listener.mostSimilar.showLabel(listener.get().toString());
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
    $("input:radio[value='by-shot']").prop('checked', true).change();

    // populate videos
    $('#show').empty();
    for(var i in videos) {
        $('#show').append($("<option></option>").attr("value", videos[i]).text(videos[i])); 
    }
    $('#show').change(function() {
        var show = $(this).val();
        video = show;
        shotIndex = {};
        callback = function() {
            $('#shots').empty();
            $('#by-label-shots').empty();
            for(var i in images) {
                var name = basename(images[i]);
                shotIndex[name] = {id: i, image: datadir + '/' + images[i]};
                // by shot
                var image = $('<img class="shot">');
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

});

%% 1) Tableau des opérations pour la parcelle 9

Tops = readtable('data_2/travail_agricole.csv');
Ta   = readtable('data_2/ITK.csv');

% Filtre parcelle 9
Tops = Tops(Tops.Plot == 9, :);

% Variables utiles et jointure avec ITK
Tops = Tops(:, {'Crop','Date_interv','Passage','ITK'});
Tops.ITK = categorical(Tops.ITK);
Ta.ITK   = categorical(Ta.ITK);

Tops = innerjoin(Tops, Ta, 'Keys','ITK');
Tops = Tops(:, {'Date_interv','Passage','PressionMax_kPa_'});

% Dates -> AAAAMMJJ
dt = datetime(Tops.Date_interv, "InputFormat","dd/MM/yyyy");
Tops.Date_interv = yyyymmdd(dt);

% Tri
Tops = sortrows(Tops, 'Date_interv');

%% 2) Compléter toutes les dates (jours sans opérations)

d     = datetime(string(Tops.Date_interv), "InputFormat","yyyyMMdd");
allDn = yyyymmdd( (min(d):max(d))' );

% Jours manquants
missing = setdiff(allDn, Tops.Date_interv);
nMiss   = numel(missing);

Tmiss = table;
Tmiss.Date_interv      = missing;
Tmiss.Passage          = zeros(nMiss,1);
Tmiss.PressionMax_kPa_ = zeros(nMiss,1);

OpsFull = [Tops; Tmiss];
OpsFull = sortrows(OpsFull, 'Date_interv');

%% 3) Ajouter le water content à partir de theta_output.csv

W = readtable('../premiere_partie_mathilde/theta_output.csv');
% Colonnes : Date_Theta, theta
W.Properties.VariableNames = {'Date_interv','WaterContent'};

% S'assurer que Date_interv est numérique AAAAMMJJ
if ~isnumeric(W.Date_interv)
    W.Date_interv = yyyymmdd( datetime(W.Date_interv,"InputFormat","yyyyMMdd") );
end

% Jointure gauche : toutes les dates de OpsFull, theta quand dispo
OpsFull = outerjoin(OpsFull, W, ...
    "Keys","Date_interv", ...
    "Type","left", ...
    "MergeKeys",true);

% Remplacer les NaN de WaterContent par 0
OpsFull.WaterContent(ismissing(OpsFull.WaterContent)) = 0;

% Écriture finale
writetable(OpsFull, "data_porosity_full.csv");

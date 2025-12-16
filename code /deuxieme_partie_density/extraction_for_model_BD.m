%% 1. Données d'opérations agricoles (parcelle 9)

Tops = readtable('data_2/travail_agricole.csv');   % opérations
numParcelle = 9;

% Filtre parcelle
Tops = Tops(Tops.Plot == numParcelle, :);

% Colonnes utiles
Tops = Tops(:, {'Crop','Date_interv','Passage','ITK'});

% Jointure avec les caractéristiques d'ITK (tracteurs)
TITK = readtable('data_2/ITK.csv');
Tops.ITK = categorical(Tops.ITK);
TITK.ITK = categorical(TITK.ITK);

Tops = innerjoin(Tops, TITK, 'Keys', 'ITK');

% On ne garde que date, passages et pression
Tops = Tops(:, {'Date_interv','Passage','PressionMax_kPa_'});

% Conversion des dates en entiers AAAAMMJJ
dtOps = datetime(Tops.Date_interv, "InputFormat","dd/MM/yyyy");
Tops.Date_interv = yyyymmdd(dtOps);

% Tri chronologique
Tops = sortrows(Tops, 'Date_interv');

%% 2. Compléter avec les jours sans opération et initialiser WaterContent

% Intervalle de dates complet
dAll = datetime(string(Tops.Date_interv), "InputFormat","yyyyMMdd");
dMin = min(dAll);
dMax = max(dAll);

allDates    = (dMin : dMax)';          % datetime
allDatesNum = yyyymmdd(allDates);      % entiers AAAAMMJJ

% Jours sans opération
[~, idxMiss] = setdiff(allDatesNum, Tops.Date_interv);
missingDates = allDatesNum(idxMiss);
nMiss        = numel(missingDates);

Tmissing = table;
Tmissing.Date_interv      = missingDates;
Tmissing.Passage          = zeros(nMiss,1);
Tmissing.PressionMax_kPa_ = zeros(nMiss,1);

% Ajouter la colonne WaterContent (0) dans les deux tables
Tops.WaterContent     = zeros(height(Tops),1);
Tmissing.WaterContent = zeros(height(Tmissing),1);

% Concaténer et trier
Tporosity = [Tops; Tmissing];
Tporosity = sortrows(Tporosity, 'Date_interv');

% Sauvegarde intermédiaire (optionnelle)
writetable(Tporosity, "data_porosity_full.csv");

%% 3. Injection du WaterContent calculé (theta_output.csv)

Ttheta = readtable("theta_output.csv");      % colonnes : Date_Theta, Theta

% Harmoniser la clé de jointure
dtTheta = datetime(string(Ttheta.Date), "InputFormat","yyyyMMdd");
Ttheta.Date_interv = yyyymmdd(dtTheta);
Ttheta.Date  = [];

% Renommer Theta -> WaterContent
Ttheta.Properties.VariableNames{'Theta'} = 'WaterContent';

% Jointure gauche : on garde toutes les lignes de Tporosity
Tmerged = outerjoin(Tporosity, Ttheta, ...
                    "Keys","Date_interv", ...
                    "MergeKeys",true, ...
                    "Type","left");

% Après outerjoin, il peut rester deux colonnes WaterContent : choisit celle de theta
if ismember('WaterContent_Tporosity', Tmerged.Properties.VariableNames)
    wc_old = Tmerged.WaterContent_Tporosity; % si tu veux les garder pour vérifier
    Tmerged.WaterContent_Tporosity = [];
end
if ismember('WaterContent_Ttheta', Tmerged.Properties.VariableNames)
    Tmerged.Properties.VariableNames{'WaterContent_Ttheta'} = 'WaterContent';
end

% Remplacer les NaN éventuels (dates sans mesure) par 0 ou autre stratégie
Tmerged.WaterContent(isnan(Tmerged.WaterContent)) = 0;

% Tri final et écriture
Tmerged = sortrows(Tmerged, 'Date_interv');
writetable(Tmerged, "data_porosity_full.csv");

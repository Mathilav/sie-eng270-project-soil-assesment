function extraction_porosity(input_file_agricultural_work, input_file_ITK, input_file_theta, output_file)
    % extraction_porosity
    %   Build a daily table of traffic (passes, pressure) and water content
    %   for plot 9 and write it to a CSV file.
    %
    % Parameters
    %   input_file_agricultural_work : path to agricultural work file (operations)
    %   input_file_ITK               : path to ITK file (implements / pressure)
    %   input_file_theta             : path to theta output file (water content)
    %   output_file                  : path to output CSV file
    
        %% 1) Operations table for plot 9
    
        Tops = readtable(input_file_agricultural_work);
        Ta   = readtable(input_file_ITK);
    
        % Keep only plot 9
        Tops = Tops(Tops.Plot == 9, :);
    
        % Keep useful variables and join with ITK to get pressure
        Tops = Tops(:, {'Crop', 'Date_interv', 'Passage', 'ITK'});
        Tops.ITK = categorical(Tops.ITK);
        Ta.ITK   = categorical(Ta.ITK);
    
        Tops = innerjoin(Tops, Ta, 'Keys', 'ITK');
        Tops = Tops(:, {'Date_interv', 'Passage', 'PressionMax_kPa_'});
    
        % Convert dates from dd/MM/yyyy text to numeric yyyymmdd
        dt = datetime(Tops.Date_interv, "InputFormat", "dd/MM/yyyy");
        Tops.Date_interv = yyyymmdd(dt);
    
        % Sort by date
        Tops = sortrows(Tops, 'Date_interv');
    
        %% 2) Add missing days (no operations)
    
        d     = datetime(string(Tops.Date_interv), "InputFormat", "yyyyMMdd");
        allDn = yyyymmdd((min(d) : max(d))');
    
        % Identify dates with no operations
        missing = setdiff(allDn, Tops.Date_interv);
        nMiss   = numel(missing);
    
        Tmiss = table;
        Tmiss.Date_interv      = missing;
        Tmiss.Passage          = zeros(nMiss, 1);
        Tmiss.PressionMax_kPa_ = zeros(nMiss, 1);
    
        OpsFull = [Tops; Tmiss];
        OpsFull = sortrows(OpsFull, 'Date_interv');
    
        %% 3) Add water content from theta_output.csv
    
        W = readtable(input_file_theta);
        % Columns expected: Date_Theta, theta
        W.Properties.VariableNames = {'Date_interv', 'WaterContent'};
    
        % Ensure Date_interv is numeric yyyymmdd
        if ~isnumeric(W.Date_interv)
            W.Date_interv = yyyymmdd( ...
                datetime(W.Date_interv, "InputFormat", "yyyyMMdd"));
        end
    
        % Left join: keep all dates from OpsFull, add water content when available
        OpsFull = outerjoin(OpsFull, W, ...
            "Keys", "Date_interv", ...
            "Type", "left", ...
            "MergeKeys", true);
    
        % Replace missing WaterContent values with 0
        OpsFull.WaterContent(ismissing(OpsFull.WaterContent)) = 0;
    
        % Write final table
        writetable(OpsFull, output_file);
    end
    
#include <fstream>
#include <iostream>

#include "MR_debug.h"
#include "WL_Instance.h"

void getPatterns(vector<vector<Supply>> patterns)
{
    for (unsigned j = 0; j < patterns.size(); j++)
    {
        for (unsigned t = 0; t < patterns[j].size(); t++)
        {
            cout << "{" << patterns[j][t].s << ", " << patterns[j][t].w << ", " << patterns[j][t].q << "}" << endl;
        }
    }
    return;
}

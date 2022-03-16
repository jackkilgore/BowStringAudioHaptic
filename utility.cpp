//
//  utility.cpp
//  basic_sound
//
//  Created by Jack Kilgore on 2/23/22.
//  Copyright © 2022 Force Dimension. All rights reserved.
//

#include "utility.hpp"



cVector3d normalize_position(cVector3d pos) {
    cVector3d result;
    auto x_norm = (pos.x() - NOVINT_X_MIN) / (NOVINT_X_MAX - NOVINT_X_MIN);
    auto y_norm = (pos.y() - NOVINT_Y_MIN) / (NOVINT_Y_MAX - NOVINT_Y_MIN);
    auto z_norm = (pos.z() - NOVINT_Z_MIN) / (NOVINT_Z_MAX - NOVINT_Z_MIN);
    result.set(x_norm,y_norm,z_norm);
    return result;
}

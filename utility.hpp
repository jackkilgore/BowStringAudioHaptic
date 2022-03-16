//
//  utility.hpp
//  basic_sound
//
//  Created by Jack Kilgore on 2/23/22.
//  Copyright © 2022 Force Dimension. All rights reserved.
//

#ifndef utility_hpp
#define utility_hpp

#include <stdio.h>
#include "chai3d.h"

using namespace chai3d;


const double NOVINT_X_MIN = -0.0374;
const double NOVINT_X_MAX = 0.0437;

const double NOVINT_Y_MIN = -0.055;
const double NOVINT_Y_MAX = 0.0635;

const double NOVINT_Z_MIN = -0.055;
const double NOVINT_Z_MAX = 0.067;

const unsigned int kMaxPacketSize = 32;


cVector3d normalize_position(cVector3d pos);

#endif /* utility_hpp */

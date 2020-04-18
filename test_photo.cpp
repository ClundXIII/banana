#include "papaya2.hpp"
#include "catch.hpp"
#include "test_helpers.hpp"
#include <sstream>

using namespace papaya2;

TEST_CASE ("width and height can be extracted")
{
    GIVEN( "a photo" )
    {
        Photo p;
        WHEN( "coordinates are set" )
        {
            p.set_coordinates (0, 0, 1, 3, 20, 40);
            THEN( "size can be extracted" )
            {
                CHECK (p.width () == 20);
                CHECK (p.height () == 40);
                CHECK (p.pixel_width () == 1./20);
                CHECK (p.pixel_height () == 3./40);
            }
        }
    }
}

// scale for TestPhoto
// choose something different from 1 so unit bugs are detectable.
static const double TP_PIXEL_SIDE = 2.;
static const double TP_PIXEL_AREA = fsq (TP_PIXEL_SIDE);

// simple photo struct for use in test cases
struct TestPhoto : BasicPhoto <int>
{
    TestPhoto (const char *data)
        : TestPhoto (string (data))
    {
    }

    TestPhoto (const string &data)
    {
        // factor image size
        size_t first__ = data.find_first_of ('_');
        if (first__ == data.npos)
            throw std::runtime_error ("no rowbreak in TestPhoto");
        numx = first__;
        if (data.size () % (numx+1u))
            throw std::runtime_error ("invalid TestPhoto");
        numy = data.size () / (numx+1u);
        if (numx == 0u || numy == 0u)
            throw std::runtime_error ("empty TestPhoto");
        set_coordinates (0., 0., TP_PIXEL_SIDE*numx, TP_PIXEL_SIDE*numy, numx, numy);
        // check data
        for (int y = 0; y != numy; ++y)
        {
            for (int x = 0; x != numx; ++x)
            {
                int pixel = unchecked_at (data, x, y);
                if (pixel != ' ' /* black */ && pixel != 'x' /* white */)
                    throw std::runtime_error ("invalid pixel in TestPhoto");
                else
                    at (x,y) = (pixel == 'x');
            }
            if (unchecked_at (data, numx, y) != '_' /* newline */)
                throw std::runtime_error ("missing rowbreak in TestPhoto");
        }
    }

private:
    int unchecked_at (const string &data, int x, int y)
    {
        return data[(numy-1-y)*(numx+1) + x];
    }
};

TEST_CASE ( "marching squares algorithm")
{
    SECTION ("all 2x2 neighborhoods")
    {
        const CApprox ref_volume[16] = {
            approx (0.), approx (.125), approx (.125), approx (.5),
            approx (.125), approx (.5), approx (.75), approx (.875),
            approx (.125), approx (.75), approx (.5), approx (.875),
            approx (.5), approx (.875), approx (.875), approx (1.)
        };
        const char *config[16] =
        {
            "  _"
            "  _",
            "  _"
            "x _",
            "x _"
            "  _",
            "x _"
            "x _",

            "  _"
            " x_",
            "  _"
            "xx_",
            "x _"
            " x_",
            "x _"
            "xx_",

            " x_"
            "  _",
            " x_"
            "x _",
            "xx_"
            "  _",
            "xx_"
            "x _",

            " x_"
            " x_",
            " x_"
            "xx_",
            "xx_"
            " x_",
            "xx_"
            "xx_"
        };

        for (int i = 0; i != 16; ++i)
        {
            INFO ("marching squares configuration " << i);
            TestPhoto ph (config[i]);
            CHECK (ph.width () == 2);
            CHECK (ph.height () == 2);
            auto i_imt = imt_interpolated_marching_squares (ph, .5, false);
            auto r_imt = imt_regular_marching_squares (ph, .5, false);
            CHECK (i_imt.area () == ref_volume[i] * TP_PIXEL_AREA);
            CHECK (r_imt.area () == ref_volume[i] * TP_PIXEL_AREA);
            CHECK_NOTHROW (check_msq_positively_oriented (ph));
        }
    }
}

TEST_CASE ("simple marching squares IMT examples")
{
    SECTION( "a single pixel" )
    {
        TestPhoto one_pixel =
            "   _"
            " x _"
            "   _";
        auto const ref_area = ( .5     ) * TP_PIXEL_AREA;
        auto const ref_peri = ( 2*SQRT2) * TP_PIXEL_SIDE;
        auto const ref_psi2 = ( 0.     ) * TP_PIXEL_SIDE;
        auto const ref_psi3 = ( 0.     ) * TP_PIXEL_SIDE;
        auto const ref_psi4 = (-2*SQRT2) * TP_PIXEL_SIDE;
        auto i_imt = imt_interpolated_marching_squares (one_pixel, .5, false);
        auto r_imt = imt_regular_marching_squares (one_pixel, .5, false);
        CHECK (i_imt.area () == approx (ref_area));
        CHECK (i_imt.msm (2) == approx (0.      ));
        CHECK (i_imt.msm (3) == approx (0.      ));
        CHECK (i_imt.msm (4) == approx (1.      ));
        CHECK (i_imt.imt (0) == approx (ref_peri));
        CHECK (i_imt.imt (2) == approx (ref_psi2));
        CHECK (i_imt.imt (3) == approx (ref_psi3));
        CHECK (i_imt.imt (4) == approx (ref_psi4));
        CHECK (r_imt.area () == approx (ref_area));
        CHECK (r_imt.msm (2) == approx (0.      ));
        CHECK (r_imt.msm (3) == approx (0.      ));
        CHECK (r_imt.msm (4) == approx (1.      ));
        CHECK (r_imt.imt (0) == approx (ref_peri));
        CHECK (r_imt.imt (2) == approx (ref_psi2));
        CHECK (r_imt.imt (3) == approx (ref_psi3));
        CHECK (r_imt.imt (4) == approx (ref_psi4));
    }
    SECTION ("tetromino1" ) {
        // reference values in pixels
        auto const ref_area =  3.5          * TP_PIXEL_AREA;
        auto const ref_peri = (4*SQRT2 + 2) * TP_PIXEL_SIDE;

        SECTION( "orientation #1" )
        {
            auto const ref_psi2 = complex_t ( 2.         , 0.) * TP_PIXEL_SIDE;
            auto const ref_psi3 = complex_t (-4.         , 0.) * TP_PIXEL_SIDE;
            auto const ref_psi4 = complex_t ( 2 - 4*SQRT2, 0.) * TP_PIXEL_SIDE;
            TestPhoto ph =
                "    _"
                " x  _"
                " xx _"
                " x  _"
                "    _";
            auto i_imt = imt_interpolated_marching_squares (ph, .5, false);
            auto r_imt = imt_regular_marching_squares (ph, .5, false);
            CHECK (i_imt.area ()      == approx (ref_area));
            CHECK (i_imt.perimeter () == approx (ref_peri));
            CHECK (i_imt.imt (0)      == approx (ref_peri));
            CHECK (i_imt.imt (2)      == approx (ref_psi2));
            CHECK (i_imt.imt (3)      == approx (ref_psi3));
            CHECK (i_imt.imt (4)      == approx (ref_psi4));
            CHECK (r_imt.area ()      == approx (ref_area));
            CHECK (r_imt.perimeter () == approx (ref_peri));
            CHECK (r_imt.imt (0)      == approx (ref_peri));
            CHECK (r_imt.imt (2)      == approx (ref_psi2));
            CHECK (r_imt.imt (3)      == approx (ref_psi3));
            CHECK (r_imt.imt (4)      == approx (ref_psi4));
        }

        SECTION( "orientation #2" )
        {
            auto const ref_psi2 = complex_t (-2.         , 0.) * TP_PIXEL_SIDE;
            auto const ref_psi3 = complex_t ( 0.         , 4.) * TP_PIXEL_SIDE;
            auto const ref_psi4 = complex_t ( 2 - 4*SQRT2, 0.) * TP_PIXEL_SIDE;
            SECTION( "test w/o padding" )
            {

                TestPhoto ph =
                    "     _"
                    "  x  _"
                    " xxx _"
                    "     _";
                auto i_imt = imt_interpolated_marching_squares (ph, .5, false);
                auto r_imt = imt_regular_marching_squares (ph, .5, false);
                CHECK (i_imt.area ()      == approx (ref_area));
                CHECK (i_imt.perimeter () == approx (ref_peri));
                CHECK (i_imt.imt (2)      == approx (ref_psi2));
                CHECK (i_imt.imt (3)      == approx (ref_psi3));
                CHECK (i_imt.imt (4)      == approx (ref_psi4));
                CHECK (r_imt.area ()      == approx (ref_area));
                CHECK (r_imt.perimeter () == approx (ref_peri));
                CHECK (r_imt.imt (2)      == approx (ref_psi2));
                CHECK (r_imt.imt (3)      == approx (ref_psi3));
                CHECK (r_imt.imt (4)      == approx (ref_psi4));
            }
            SECTION( "orientation #2, test padding" )
            {
                TestPhoto ph =
                    "    _"
                    " x  _"
                    "xxx _";
                auto i_imt = imt_interpolated_marching_squares (ph, .5, true);
                auto r_imt = imt_regular_marching_squares (ph, .5, true);
                CHECK (i_imt.area ()      == approx (ref_area));
                CHECK (i_imt.perimeter () == approx (ref_peri));
                CHECK (i_imt.imt (2)      == approx (ref_psi2));
                CHECK (i_imt.imt (3)      == approx (ref_psi3));
                CHECK (i_imt.imt (4)      == approx (ref_psi4));
                CHECK (r_imt.area ()      == approx (ref_area));
                CHECK (r_imt.perimeter () == approx (ref_peri));
                CHECK (r_imt.imt (2)      == approx (ref_psi2));
                CHECK (r_imt.imt (3)      == approx (ref_psi3));
                CHECK (r_imt.imt (4)      == approx (ref_psi4));
            }
            SECTION( "orientation #2, test other padding" )
            {
                TestPhoto ph =
                    "    _"
                    "  x _"
                    " xxx_";
                auto i_imt = imt_interpolated_marching_squares (ph, .5, true);
                auto r_imt = imt_regular_marching_squares (ph, .5, true);
                CHECK (i_imt.area ()      == approx (ref_area));
                CHECK (i_imt.perimeter () == approx (ref_peri));
                CHECK (i_imt.imt (2)      == approx (ref_psi2));
                CHECK (i_imt.imt (3)      == approx (ref_psi3));
                CHECK (i_imt.imt (4)      == approx (ref_psi4));
                CHECK (r_imt.area ()      == approx (ref_area));
                CHECK (r_imt.perimeter () == approx (ref_peri));
                CHECK (r_imt.imt (2)      == approx (ref_psi2));
                CHECK (r_imt.imt (3)      == approx (ref_psi3));
                CHECK (r_imt.imt (4)      == approx (ref_psi4));
            }
        }
    }
    SECTION( "corner case where the threshold matches a pixel value exactly" )
    {
        TestPhoto one_pixel =
            "   _"
            " x _"
            "   _";
        auto const i_imt = imt_interpolated_marching_squares (one_pixel, 1., false);
        CHECK (i_imt.area () == 0.);
        CHECK (i_imt.imt (0) == 0.);
        CHECK (i_imt.imt (2) == 0.);
        CHECK (i_imt.imt (3) == 0.);
        CHECK (i_imt.imt (4) == 0.);
    }
}
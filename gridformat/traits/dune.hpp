// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup PredefinedTraits
 * \brief Traits specializations for <a href="https://gitlab.dune-project.org/core/dune-grid">dune grid views</a>
 */
#ifndef GRIDFORMAT_TRAITS_DUNE_HPP_
#define GRIDFORMAT_TRAITS_DUNE_HPP_

#include <ranges>
#include <cassert>
// dune seems to not explicitly include this but uses std::int64_t.
// With gcc-13 this leads to an error, maybe before gcc-13 the header
// was included by some other standard library header...
#include <cstdint>

#ifdef GRIDFORMAT_IGNORE_DUNE_WARNINGS
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#pragma GCC diagnostic ignored "-Wnull-dereference"
#endif  // GRIDFORMAT_IGNORE_DUNE_WARNINGS
#include <dune/geometry/type.hh>
#include <dune/grid/common/gridview.hh>
#include <dune/grid/common/gridenums.hh>
#include <dune/grid/yaspgrid.hh>
#ifdef GRIDFORMAT_IGNORE_DUNE_WARNINGS
#pragma GCC diagnostic pop
#endif  // GRIDFORMAT_IGNORE_DUNE_WARNINGS

#include <gridformat/common/ranges.hpp>
#include <gridformat/common/exceptions.hpp>

#include <gridformat/grid/cell_type.hpp>
#include <gridformat/grid/traits.hpp>

namespace GridFormat::Traits {

#ifndef DOXYGEN
namespace DuneDetail {

inline int map_corner_index(const Dune::GeometryType& gt, int i) {
    if (gt.isQuadrilateral()) {
        assert(i < 4);
        static constexpr int map[4] = {0, 1, 3, 2};
        return map[i];
    }
    if (gt.isHexahedron()) {
        assert(i < 8);
        static constexpr int map[8] = {0, 1, 3, 2, 4, 5, 7, 6};
        return map[i];
    }
    return i;
}

inline constexpr GridFormat::CellType cell_type(const Dune::GeometryType& gt) {
    if (gt.isVertex()) return GridFormat::CellType::vertex;
    if (gt.isLine()) return GridFormat::CellType::segment;
    if (gt.isTriangle()) return GridFormat::CellType::triangle;
    if (gt.isQuadrilateral()) return GridFormat::CellType::quadrilateral;
    if (gt.isTetrahedron()) return GridFormat::CellType::tetrahedron;
    if (gt.isHexahedron()) return GridFormat::CellType::hexahedron;
    throw NotImplemented("Unknown Dune::GeometryType");
}

template<typename GridView>
using Element = typename GridView::template Codim<0>::Entity;

template<typename GridView>
using Vertex = typename GridView::template Codim<GridView::dimension>::Entity;

}  // namespace DuneDetail
#endif  // DOXYGEN


template<typename Traits>
struct Points<Dune::GridView<Traits>> {
    static decltype(auto) get(const Dune::GridView<Traits>& grid_view) {
        using GV = Dune::GridView<Traits>;
        static constexpr int vertex_codim = GV::dimension;
        return std::ranges::subrange(
            grid_view.template begin<vertex_codim, Dune::InteriorBorder_Partition>(),
            grid_view.template end<vertex_codim, Dune::InteriorBorder_Partition>()
        );
    }
};

template<typename Traits>
struct Cells<Dune::GridView<Traits>> {
    static decltype(auto) get(const Dune::GridView<Traits>& grid_view) {
        return std::ranges::subrange(
            grid_view.template begin<0, Dune::Interior_Partition>(),
            grid_view.template end<0, Dune::Interior_Partition>()
        );
    }
};

template<typename Traits>
struct NumberOfPoints<Dune::GridView<Traits>> {
    static auto get(const Dune::GridView<Traits>& grid_view) {
        static constexpr int point_codim = Dune::GridView<Traits>::dimension;
        if (grid_view.comm().size() == 1)
            return static_cast<std::size_t>(grid_view.size(point_codim));
        return static_cast<std::size_t>(
            Ranges::size(Points<Dune::GridView<Traits>>::get(grid_view))
        );
    }
};

template<typename Traits>
struct NumberOfCells<Dune::GridView<Traits>> {
    static auto get(const Dune::GridView<Traits>& grid_view) {
        if (grid_view.comm().size() == 1)
            return static_cast<std::size_t>(grid_view.size(0));
        return static_cast<std::size_t>(
            Ranges::size(Cells<Dune::GridView<Traits>>::get(grid_view))
        );
    }
};

template<typename Traits>
struct NumberOfCellPoints<Dune::GridView<Traits>, DuneDetail::Element<Dune::GridView<Traits>>> {
    static auto get(const Dune::GridView<Traits>&,
                    const DuneDetail::Element<Dune::GridView<Traits>>& cell) {
        return cell.subEntities(Dune::GridView<Traits>::dimension);
    }
};

template<typename Traits>
struct CellPoints<Dune::GridView<Traits>, DuneDetail::Element<Dune::GridView<Traits>>> {
    static decltype(auto) get(const Dune::GridView<Traits>&,
                              const DuneDetail::Element<Dune::GridView<Traits>>& element) {
        static constexpr int dim = Dune::GridView<Traits>::dimension;
        return std::views::iota(unsigned{0}, element.subEntities(dim)) | std::views::transform([&] (int i) {
            return element.template subEntity<dim>(DuneDetail::map_corner_index(element.type(), i));
        });
    }
};

template<typename Traits>
struct CellType<Dune::GridView<Traits>, DuneDetail::Element<Dune::GridView<Traits>>> {
    static decltype(auto) get(const Dune::GridView<Traits>&,
                              const DuneDetail::Element<Dune::GridView<Traits>>& element) {
        return DuneDetail::cell_type(element.type());
    }
};

template<typename Traits>
struct PointCoordinates<Dune::GridView<Traits>, DuneDetail::Vertex<Dune::GridView<Traits>>> {
    static decltype(auto) get(const Dune::GridView<Traits>&,
                              const DuneDetail::Vertex<Dune::GridView<Traits>>& vertex) {
        return vertex.geometry().center();
    }
};

template<typename Traits>
struct PointId<Dune::GridView<Traits>, DuneDetail::Vertex<Dune::GridView<Traits>>> {
    static decltype(auto) get(const Dune::GridView<Traits>& grid_view,
                              const DuneDetail::Vertex<Dune::GridView<Traits>>& vertex) {
        return grid_view.indexSet().index(vertex);
    }
};


#ifndef DOXYGEN
namespace DuneDetail {

    template<typename T>
    struct IsDuneYaspGrid : public std::false_type {};

    template<int dim, typename Coords>
    struct IsDuneYaspGrid<Dune::YaspGrid<dim, Coords>> : public std::true_type {
        static constexpr bool uses_tp_coords = std::same_as<
            Coords,
            Dune::TensorProductCoordinates<typename Coords::ctype, dim>
        >;
    };

    template<typename GridView>
    inline constexpr bool is_yasp_grid_view = IsDuneYaspGrid<typename GridView::Grid>::value;

    template<typename GridView> requires(is_yasp_grid_view<GridView>)
    inline constexpr bool uses_tensor_product_coords = IsDuneYaspGrid<typename GridView::Grid>::uses_tp_coords;

    template<typename ctype, int dim>
    auto spacing_in(int direction, const Dune::EquidistantCoordinates<ctype, dim>& coords) {
        return coords.meshsize(direction, 0);
    }
    template<typename ctype, int dim>
    auto spacing_in(int direction, const Dune::EquidistantOffsetCoordinates<ctype, dim>& coords) {
        return coords.meshsize(direction, 0);
    }

}  // namespace DuneDetail
#endif  // DOXYGEN


// Register YaspGrid as structured grid
template<typename Traits> requires(DuneDetail::is_yasp_grid_view<Dune::GridView<Traits>>)
struct Extents<Dune::GridView<Traits>> {
    static auto get(const Dune::GridView<Traits>& grid_view) {
        const auto level = std::ranges::begin(Cells<Dune::GridView<Traits>>::get(grid_view))->level();
        const auto& grid_level = *grid_view.grid().begin(level);
        const auto& interior_grid = grid_level.interior[0];
        const auto& gc = *interior_grid.dataBegin();

        static constexpr int dim = Traits::Grid::dimension;
        std::array<std::size_t, dim> result;
        for (int i = 0; i < Traits::Grid::dimension; ++i)
            result[i] = gc.max(i) - gc.min(i) + 1;
        return result;
    }
};

template<typename Traits, typename Entity> requires(DuneDetail::is_yasp_grid_view<Dune::GridView<Traits>>)
struct Location<Dune::GridView<Traits>, Entity> {
    static auto get(const Dune::GridView<Traits>& grid_view, const Entity& entity) {
        auto const& grid_level = *grid_view.grid().begin(entity.level());
        auto const& interior = grid_level.interior[0];
        auto const& extent_bounds = *interior.dataBegin();

        auto result = entity.impl().transformingsubiterator().coord();
        for (int i = 0; i < Dune::GridView<Traits>::dimension; ++i)
            result[i] -= extent_bounds.min(i);
        return result;
    }
};

template<typename Traits> requires(DuneDetail::is_yasp_grid_view<Dune::GridView<Traits>>)
struct Origin<Dune::GridView<Traits>> {
    static auto get(const Dune::GridView<Traits>& grid_view) {
        const auto level = std::ranges::begin(Cells<Dune::GridView<Traits>>::get(grid_view))->level();
        auto const& grid_level = *grid_view.grid().begin(level);
        auto const& interior = grid_level.interior[0];
        auto const& extent_bounds = *interior.dataBegin();

        std::array<typename Traits::Grid::ctype, Traits::Grid::dimension> result;
        for (int i = 0; i < Traits::Grid::dimension; ++i)
            result[i] = grid_level.coords.coordinate(i, extent_bounds.min(i));
        return result;
    }
};

template<typename Traits>
    requires(DuneDetail::is_yasp_grid_view<Dune::GridView<Traits>> and
             !DuneDetail::uses_tensor_product_coords<Dune::GridView<Traits>>) // spacing not uniquely defined for tpcoords
struct Spacing<Dune::GridView<Traits>> {
    static auto get(const Dune::GridView<Traits>& grid_view) {
        const auto level = std::ranges::begin(Cells<Dune::GridView<Traits>>::get(grid_view))->level();
        auto const& grid_level = *grid_view.grid().begin(level);

        std::array<typename Traits::Grid::ctype, Traits::Grid::dimension> result;
        for (int i = 0; i < Traits::Grid::dimension; ++i)
            result[i] = DuneDetail::spacing_in(i, grid_level.coords);
        return result;
    }
};

template<typename Traits> requires(DuneDetail::is_yasp_grid_view<Dune::GridView<Traits>>)
struct Ordinates<Dune::GridView<Traits>> {
    static auto get(const Dune::GridView<Traits>& grid_view, int direction) {
        const auto level = std::ranges::begin(Cells<Dune::GridView<Traits>>::get(grid_view))->level();
        auto const& grid_level = *grid_view.grid().begin(level);
        auto const& interior = grid_level.interior[0];
        auto const& extent_bounds = *interior.dataBegin();

        const auto num_point_ordinates = extent_bounds.max(direction) - extent_bounds.min(direction) + 2;
        std::vector<typename Traits::Grid::ctype> ordinates(num_point_ordinates);
        for (int i = 0; i < num_point_ordinates; ++i)
            ordinates[i] = grid_level.coords.coordinate(direction, extent_bounds.min(direction) + i);
        return ordinates;
    }
};

}  // namespace GridFormat::Traits

#if GRIDFORMAT_HAVE_DUNE_LOCALFUNCTIONS

#include <map>
#include <unordered_map>
#include <iterator>

#ifdef GRIDFORMAT_IGNORE_DUNE_WARNINGS
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif  // GRIDFORMAT_IGNORE_DUNE_WARNINGS
#include <dune/geometry/multilineargeometry.hh>
#include <dune/geometry/referenceelements.hh>

#include <dune/grid/common/mcmgmapper.hh>
#include <dune/localfunctions/lagrange/equidistantpoints.hh>
#ifdef GRIDFORMAT_IGNORE_DUNE_WARNINGS
#pragma GCC diagnostic pop
#endif  // GRIDFORMAT_IGNORE_DUNE_WARNINGS


namespace GridFormat {

#ifndef DOXYGEN
namespace DuneLagrangeDetail {

    inline int dune_to_gfmt_sub_entity(const Dune::GeometryType& gt, int i, int codim) {
        if (gt.isTriangle()) {
            if (codim == 1) {
                assert(i < 3);
                static constexpr int map[3] = {0, 2, 1};
                return map[i];
            }
        }
        if (gt.isQuadrilateral()) {
            if (codim == 2) {
                assert(i < 4);
                static constexpr int map[4] = {0, 1, 3, 2};
                return map[i];
            }
            if (codim == 1) {
                assert(i < 4);
                static constexpr int map[4] = {3, 1, 0, 2};
                return map[i];
            }
        }
        if (gt.isTetrahedron()) {
            if (codim == 2) {
                assert(i < 6);
                static constexpr int map[6] = {0, 2, 1, 3, 4, 5};
                return map[i];
            }
            if (codim == 1) {
                assert(i < 4);
                static constexpr int map[4] = {3, 0, 2, 1};
                return map[i];
            }
        }
        if (gt.isHexahedron()) {
            if (codim == 3) {
                assert(i < 8);
                static constexpr int map[8] = {0, 1, 3, 2, 4, 5, 7, 6};
                return map[i];
            }
            if (codim == 2) {
                assert(i < 12);
                static constexpr int map[12] = {8, 9, 11, 10, 3, 1, 0, 2, 7, 5, 4, 6};
                return map[i];
            }
        }
        return i;
    }

    inline constexpr GridFormat::CellType cell_type(const Dune::GeometryType& gt) {
        if (gt.isLine()) return GridFormat::CellType::lagrange_segment;
        if (gt.isTriangle()) return GridFormat::CellType::lagrange_triangle;
        if (gt.isQuadrilateral()) return GridFormat::CellType::lagrange_quadrilateral;
        if (gt.isTetrahedron()) return GridFormat::CellType::lagrange_tetrahedron;
        if (gt.isHexahedron()) return GridFormat::CellType::lagrange_hexahedron;
        throw NotImplemented("Unsupported Dune::GeometryType");
    }

}  // namespace DuneLagrangeDetail
#endif  // DOXYGEN

/*!
 * \ingroup PredefinedTraits
 * \brief Exposes a `Dune::GridView` as a mesh composed of lagrange cells with the given order.
 *        Can be used to conveniently write `Dune::Functions` into mesh files.
 */
template<typename GridView>
class DuneLagrangeMesh {
    using Element = typename GridView::template Codim<0>::Entity;
    using Position = typename Element::Geometry::GlobalCoordinate;
    using LocalPoints = Dune::EquidistantPointSet<typename GridView::ctype, GridView::dimension>;
    static constexpr int dim = GridView::dimension;

    class PointIndicesHelper {
     public:
        struct Key {
            unsigned int codim;
            std::size_t global_index;
            unsigned int sub_index;
        };

        bool contains(const Key& key) const {
            if (!_codim_to_global_indices.contains(key.codim))
                return false;
            if (!_codim_to_global_indices.at(key.codim).contains(key.global_index))
                return false;
            return _codim_to_global_indices.at(key.codim).at(key.global_index).contains(key.sub_index);
        }

        void insert(const Key& key, std::size_t index) {
            _codim_to_global_indices[key.codim][key.global_index][key.sub_index] = index;
        }

        const std::ranges::range auto& get(int codim, std::size_t global_index) const {
            return _codim_to_global_indices.at(codim).at(global_index);
        }

     private:
        std::unordered_map<
            int, // codim
            std::unordered_map<
                std::size_t, // entity index
                std::unordered_map<int, std::size_t>  // sub-entity index to global indices
            >
        > _codim_to_global_indices;
    };

 public:
    DuneLagrangeMesh(const GridView& grid_view, unsigned int order = 1)
    : _grid_view{grid_view}
    , _order{order} {
        update(grid_view);
        if (order < 1)
            throw InvalidState("Order must be >= 1");
    }

    void update(const GridView& grid_view) {
        clear();
        _grid_view = grid_view;
        _update_local_points();
        _update_mesh();
    }

    void clear() {
        _local_points.clear();
        _points.clear();
        _cells.clear();
        _cell_topology_id.clear();
    }

    std::size_t number_of_points() const { return _points.size(); }
    std::size_t number_of_cells() const { return _cells.size(); }
    const Position& point(std::size_t i) const { return _points.at(i); }
    const std::vector<std::size_t>& cell_points(std::size_t i) const { return _cells.at(i); }

    Dune::GeometryType geometry_type(std::size_t i) const {
        return {_cell_topology_id.at(i), dim};
    }

    auto geometry(std::size_t i) const {
        const auto gt = geometry_type(i);
        const auto num_corners = referenceElement<typename GridView::ctype, dim>(gt).size(dim);
        std::vector<Position> corners; corners.reserve(num_corners);
        std::ranges::for_each(std::views::iota(0, num_corners), [&] (int corner) {
            corners.push_back(_points[_cells[i][corner]]);
        });
        return Dune::MultiLinearGeometry<typename GridView::ctype, dim, GridView::dimensionworld>{gt, corners};
    }

 private:
    void _update_local_points() {
        for (const auto& gt : _grid_view.indexSet().types(0)) {
            _local_points.emplace(gt, _order);
            _local_points.at(gt).build(gt);
        }
    }

    auto _make_codim_mappers() {
        using Mapper = Dune::MultipleCodimMultipleGeomTypeMapper<GridView>;
        std::unordered_map<int, Mapper> codim_to_mapper;
        codim_to_mapper.emplace(0, Mapper{_grid_view, Dune::mcmgLayout(Dune::Codim<0>{})});
        codim_to_mapper.emplace(1, Mapper{_grid_view, Dune::mcmgLayout(Dune::Codim<1>{})});
        if constexpr (int(GridView::dimension) >= 2)
            codim_to_mapper.emplace(2, Mapper{_grid_view, Dune::mcmgLayout(Dune::Codim<2>{})});
        if constexpr (int(GridView::dimension) == 3)
            codim_to_mapper.emplace(3, Mapper{_grid_view, Dune::mcmgLayout(Dune::Codim<3>{})});
        return codim_to_mapper;
    }

    void _update_mesh() {
        const auto codim_to_mapper = _make_codim_mappers();
        PointIndicesHelper point_indices;
        _make_points(point_indices, codim_to_mapper);
        _set_connectivity(point_indices, codim_to_mapper);
    }

    template<typename Mappers>
    void _make_points(PointIndicesHelper& point_indices, const Mappers& codim_to_mapper) {
        for (const auto& element : Traits::Cells<GridView>::get(_grid_view)) {
            for (const auto& local_point : _local_points.at(element.type())) {
                const auto codim = local_point.localKey().codim();
                const auto sub_entity = local_point.localKey().subEntity();
                const auto sub_index = local_point.localKey().index();
                const auto global_index = codim_to_mapper.at(codim).subIndex(element, sub_entity, codim);
                if (!point_indices.contains({codim, global_index, sub_index})) {
                    _points.emplace_back(element.geometry().global(local_point.point()));
                    point_indices.insert({codim, global_index, sub_index}, _points.size() - 1);
                }
            }
        }
    }

    template<typename Mappers>
    void _set_connectivity(PointIndicesHelper& point_indices, const Mappers& codim_to_mapper) {
        _cells.reserve(Traits::NumberOfCells<GridView>::get(_grid_view));
        _cell_topology_id.reserve(_cells.size());
        std::vector<std::vector<std::size_t>> codim_points;
        for (const auto& element : Traits::Cells<GridView>::get(_grid_view)) {
            _cell_topology_id.push_back(element.type().id());
            auto& cell_points = _cells.emplace_back();
            for (int codim = GridView::dimension; codim >= 0; --codim) {
                codim_points.clear();
                codim_points.resize(element.subEntities(codim));
                for (unsigned int sub_entity = 0; sub_entity < element.subEntities(codim); ++sub_entity) {
                    const auto mapped_sub_entity = DuneLagrangeDetail::dune_to_gfmt_sub_entity(
                        element.type(), sub_entity, codim
                    );
                    const auto global_index = codim_to_mapper.at(codim).subIndex(element, sub_entity, codim);
                    if (point_indices.contains({static_cast<unsigned int>(codim), global_index, 0})) {
                        const auto& indices = point_indices.get(codim, global_index);
                        codim_points[mapped_sub_entity].resize(indices.size());
                        for (const auto& [local, global] : indices)
                            codim_points[mapped_sub_entity][local] = global;
                    }
                }
                for (const auto& sub_points : codim_points) {
                    cell_points.reserve(cell_points.size() + sub_points.size());
                    std::ranges::copy(sub_points, std::back_inserter(cell_points));
                }
            }
        }
    }

    GridView _grid_view;
    unsigned int _order;
    std::vector<Position> _points;
    std::vector<std::vector<std::size_t>> _cells;
    std::vector<unsigned int> _cell_topology_id;
    std::map<Dune::GeometryType, LocalPoints> _local_points;
};

namespace Traits {

template<typename GridView>
struct Points<DuneLagrangeMesh<GridView>> {
    static decltype(auto) get(const DuneLagrangeMesh<GridView>& mesh) {
        return std::views::iota(std::size_t{0}, mesh.number_of_points());
    }
};

template<typename GridView>
struct Cells<DuneLagrangeMesh<GridView>> {
    static decltype(auto) get(const DuneLagrangeMesh<GridView>& mesh) {
        return std::views::iota(std::size_t{0}, mesh.number_of_cells());
    }
};

template<typename GridView>
struct NumberOfPoints<DuneLagrangeMesh<GridView>> {
    static auto get(const DuneLagrangeMesh<GridView>& mesh) {
        return mesh.number_of_points();
    }
};

template<typename GridView>
struct NumberOfCells<DuneLagrangeMesh<GridView>> {
    static auto get(const DuneLagrangeMesh<GridView>& mesh) {
        return mesh.number_of_cells();
    }
};

template<typename GridView>
struct NumberOfCellPoints<DuneLagrangeMesh<GridView>, std::size_t> {
    static auto get(const DuneLagrangeMesh<GridView>& mesh, const std::size_t cell_index) {
        return mesh.cell_points(cell_index).size();
    }
};

template<typename GridView>
struct CellPoints<DuneLagrangeMesh<GridView>, std::size_t> {
    static auto get(const DuneLagrangeMesh<GridView>& mesh, const std::size_t cell_index) {
        return mesh.cell_points(cell_index);
    }
};

template<typename GridView>
struct CellType<DuneLagrangeMesh<GridView>, std::size_t> {
    static decltype(auto) get(const DuneLagrangeMesh<GridView>& mesh, const std::size_t cell_index) {
        return DuneLagrangeDetail::cell_type(mesh.geometry_type(cell_index));
    }
};

template<typename GridView>
struct PointCoordinates<DuneLagrangeMesh<GridView>, std::size_t> {
    static decltype(auto) get(const DuneLagrangeMesh<GridView>& mesh, const std::size_t point_index) {
        return mesh.point(point_index);
    }
};

template<typename GridView>
struct PointId<DuneLagrangeMesh<GridView>, std::size_t> {
    static decltype(auto) get(const DuneLagrangeMesh<GridView>&, const std::size_t point_index) {
        return point_index;
    }
};

}  // namespace Traits

}  // namespace GridFormat

#endif  // GRIDFORMAT_HAVE_DUNE_LOCALFUNCTIONS
#endif  // GRIDFORMAT_TRAITS_DUNE_HPP_

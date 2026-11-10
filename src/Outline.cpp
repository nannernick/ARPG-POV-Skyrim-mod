// Feeds the point-and-click targets (enemies, NPCs, the player, items, whatever the cursor
// is on) to the outline renderer in src/render.
//
// The renderer, and the structure of the per-frame code below (render-target lookup, frame
// gate, geometry cache handling), come from Highlight Lootable Corpses by AmazingBuff, GPL-3.0:
// https://github.com/AmazingBuff/HighlightLootableCorpses

#include "Outline.h"
#include "Game.h"
#include "Settings.h"

#include "config/config.h"
#include "render/dx11/common_states.h"
#include "render/geometry/render_geometry_cache.h"
#include "render/mask/mask_overlay.h"
#include "render/shader_manager.h"
#include "render/ui_render_hook.h"

namespace Outline
{
	namespace
	{
		using namespace PLUGIN_NAMESPACE;

		// Each outlined thing costs a few draw calls, so there's a ceiling. Nearest and most
		// important first (see Priority).
		constexpr std::size_t kMaxTargets = 64;
		constexpr std::size_t kMaxGeometries = Max_Render_Geometries_Per_Frame;

		std::atomic<bool> g_hooked{ false };
		std::atomic<bool> g_failed{ false };

		struct Entry
		{
			RE::FormID        formID;
			RE::NiPoint3      centre;
			float             radius;
			float             distance;
			int               priority;
			bool              living;
			DirectX::XMFLOAT4 color;
		};

		bool Wanted(const Game::Target& a_target, bool a_hovered)
		{
			if (a_hovered) {
				return true;  // whatever the cursor is on is always outlined
			}
			if (!Settings::Outlines) {
				return false;
			}
			switch (a_target.kind) {
			case Game::Kind::kEnemy:
				return Settings::OutlineEnemies && a_target.distance <= Settings::OutlineRadius;
			case Game::Kind::kNpc:
			case Game::Kind::kPlayer:
				return Settings::OutlineNpcs && a_target.distance <= Settings::OutlineRadius;
			case Game::Kind::kItem:
				return Settings::OutlineItems && a_target.distance <= Settings::OutlineItemRadius;
			default:
				return false;
			}
		}

		int Priority(const Game::Target& a_target, bool a_hovered)
		{
			if (a_hovered) {
				return 0;
			}
			switch (a_target.kind) {
			case Game::Kind::kEnemy:
				return 1;
			case Game::Kind::kPlayer:
				return 2;
			case Game::Kind::kNpc:
				return 3;
			default:
				return 4;
			}
		}

		// The size of the image the interface is drawn into (from Highlight Lootable Corpses).
		bool TargetDimensions(REX::W32::ID3D11RenderTargetView* a_target, std::uint32_t& a_width, std::uint32_t& a_height)
		{
			if (!a_target) {
				return false;
			}
			REX::W32::D3D11_RENDER_TARGET_VIEW_DESC viewDesc{};
			a_target->GetDesc(&viewDesc);

			std::uint32_t mip = 0;
			switch (viewDesc.viewDimension) {
			case REX::W32::D3D11_RTV_DIMENSION_TEXTURE2D:
				mip = viewDesc.texture2D.mipSlice;
				break;
			case REX::W32::D3D11_RTV_DIMENSION_TEXTURE2DARRAY:
				mip = viewDesc.texture2DArray.mipSlice;
				break;
			case REX::W32::D3D11_RTV_DIMENSION_TEXTURE2DMS:
			case REX::W32::D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY:
				break;
			default:
				return false;
			}

			REX::W32::ID3D11Resource* resource = nullptr;
			a_target->GetResource(&resource);
			if (!resource) {
				return false;
			}
			REX::W32::ID3D11Texture2D* texture = nullptr;
			const REX::W32::HRESULT    hr = resource->QueryInterface(REX::W32::IID_ID3D11Texture2D, reinterpret_cast<void**>(&texture));
			resource->Release();
			if (!REX::W32::SUCCESS(hr) || !texture) {
				return false;
			}
			REX::W32::D3D11_TEXTURE2D_DESC textureDesc{};
			texture->GetDesc(&textureDesc);
			texture->Release();
			if (mip >= textureDesc.mipLevels || mip >= 32) {
				return false;
			}
			a_width = std::max(1u, textureDesc.width >> mip);
			a_height = std::max(1u, textureDesc.height >> mip);
			return true;
		}

		std::size_t CountGeometries(RE::NiAVObject* a_root)
		{
			std::size_t count = 0;
			RE::BSVisit::TraverseScenegraphGeometries(a_root, [&](RE::BSGeometry*) {
				++count;
				return RE::BSVisit::BSVisitControl::kContinue;
			});
			return count;
		}

		class Director
		{
		public:
			static Director& Get()
			{
				static Director s_instance;
				return s_instance;
			}

			void OnPreUiDraw()
			{
				// Once per rendered frame, and never on frame 0.
				auto*               state = RE::BSGraphics::State::GetSingleton();
				const std::uint32_t frame = state ? state->GetFrameCount() : 0;
				if (frame == _lastFrame || frame == 0) {
					return;
				}
				_lastFrame = frame;

				if (g_failed.load() || !State::gameplay.load()) {
					return;
				}

				auto* renderer = RE::BSGraphics::Renderer::GetSingleton();
				if (!renderer) {
					return;
				}
				auto&                          data = renderer->GetRuntimeData();
				REX::W32::ID3D11Device*        device = data.forwarder;
				REX::W32::ID3D11DeviceContext* context = data.context;
				if (!device || !context) {
					return;
				}
				REX::W32::ID3D11RenderTargetView* const output = data.renderTargets[RE::RENDER_TARGETS::kFRAMEBUFFER].RTV;
				std::uint32_t                           width = 0;
				std::uint32_t                           height = 0;
				if (!TargetDimensions(output, width, height)) {
					return;
				}
				Draw(device, context, output, width, height, frame);
			}

		private:
			bool Init(REX::W32::ID3D11Device* a_device)
			{
				if (_ready) {
					return true;
				}
				if (!ShaderManager::instance().compile()) {
					logs::error("[outline] the outline shaders didn't compile; outlines are off");
					g_failed.store(true);
					return false;
				}
				_states = std::make_unique<CommonStates>(a_device);
				if (!_states || !_states->valid() || !_mask.init(a_device)) {
					logs::error("[outline] couldn't set up the outline renderer; outlines are off");
					g_failed.store(true);
					return false;
				}
				_ready = true;
				logs::info("[outline] renderer ready");
				return true;
			}

			void Draw(REX::W32::ID3D11Device* a_device, REX::W32::ID3D11DeviceContext* a_context, REX::W32::ID3D11RenderTargetView* a_output,
				std::uint32_t a_width, std::uint32_t a_height, std::uint32_t a_frame)
			{
				if (!Init(a_device)) {
					return;
				}
				auto* camera = RE::Main::WorldRootCamera();
				if (!camera) {
					return;
				}

				std::shared_ptr<const std::vector<Game::Target>> targets;
				RE::ObjectRefHandle                              hovered;
				{
					std::scoped_lock guard(State::lock);
					targets = State::targets;
					hovered = State::hovered;
				}
				if (!targets || targets->empty()) {
					return;
				}

				// What to outline this frame.
				std::vector<Entry> entries;
				entries.reserve(targets->size());
				for (const auto& target : *targets) {
					if (target.formID == 0 || target.points.empty()) {
						continue;
					}
					const bool hot = hovered && target.handle == hovered;
					if (!Wanted(target, hot)) {
						continue;
					}
					Entry entry{};
					for (const auto& point : target.points) {
						entry.centre += point;
					}
					entry.centre /= static_cast<float>(target.points.size());
					for (const auto& point : target.points) {
						entry.radius = std::max(entry.radius, entry.centre.GetDistance(point));
					}
					entry.radius += target.pad + 20.0f;
					if (!camera->PointInFrustum(entry.centre, entry.radius)) {
						continue;
					}
					entry.formID = target.formID;
					entry.distance = target.distance;
					entry.priority = Priority(target, hot);
					entry.living = target.kind == Game::Kind::kEnemy || target.kind == Game::Kind::kNpc || target.kind == Game::Kind::kPlayer;
					entry.color = { static_cast<float>(target.color & 0xFF) / 255.0f, static_cast<float>((target.color >> 8) & 0xFF) / 255.0f,
						static_cast<float>((target.color >> 16) & 0xFF) / 255.0f, 1.0f };
					entries.push_back(entry);
				}
				if (entries.empty()) {
					return;
				}
				std::sort(entries.begin(), entries.end(), [](const Entry& a_lhs, const Entry& a_rhs) {
					return a_lhs.priority != a_rhs.priority ? a_lhs.priority < a_rhs.priority : a_lhs.distance < a_rhs.distance;
				});
				if (entries.size() > kMaxTargets) {
					entries.resize(kMaxTargets);
				}

				auto& config = Setting::instance().get_config();
				config.display_mode = Config::DisplayMode::e_outline;
				config.outline_thickness = std::clamp(Settings::OutlineThickness, Setting::Min_Outline_Thickness, Setting::Max_Outline_Thickness);

				if (!_mask.begin_frame(a_device, a_width, a_height)) {
					_mask.end_frame();
					return;
				}

				auto& cache = RenderGeometryCache::instance();
				cache.drain_invalidations();
				if (_counts.size() > 1024) {
					_counts.clear();
				}

				std::vector<DirectX::XMFLOAT4>           colors;
				std::vector<std::vector<RenderGeometry>> geometries;
				colors.reserve(entries.size());
				geometries.reserve(entries.size());

				std::uint32_t index = 0;
				std::size_t   total = 0;
				for (const auto& entry : entries) {
					auto*                    form = RE::TESForm::LookupByID(entry.formID);
					const RE::TESObjectREFR* ref = form ? form->AsReference() : nullptr;
					RE::NiAVObject* const    root = ref ? ref->GetCurrent3D() : nullptr;
					if (!ref || !root) {
						continue;
					}

					std::vector<RenderGeometry> group;
					const auto                  cached = cache.lookup(entry.formID);
					if (cached.geometries) {
						bool stale = cached.root3d != root;
						if (!stale && entry.living) {
							// Living things change: a weapon drawn or sheathed swaps parts of the model
							// without rebuilding it. A cached part that has been taken off the model
							// (no parent) would be drawn frozen in mid-air, so check every frame; added
							// parts are caught by a part count, checked about once a second.
							for (const auto& part : *cached.geometries) {
								if (part.node && !part.node->parent) {
									stale = true;
									break;
								}
							}
							if (!stale && (a_frame + entry.formID) % 45 == 0) {
								const auto it = _counts.find(entry.formID);
								stale = it == _counts.end() || it->second != CountGeometries(root);
							}
						}
						if (stale) {
							cache.erase(entry.formID);
						} else {
							group = *cached.geometries;
						}
					}
					if (group.empty()) {
						collect_render_geometries(ref, a_device, group);
						_counts[entry.formID] = CountGeometries(root);
						cache.insert(entry.formID, root, std::vector<RenderGeometry>(group));
						if (group.empty()) {
							continue;
						}
						// The cache owns the position buffers; this frame's copy only borrows them.
					}

					if (total + group.size() > kMaxGeometries) {
						break;
					}
					total += group.size();
					for (auto& part : group) {
						part.target_index = index;
					}
					colors.push_back(entry.color);
					geometries.push_back(std::move(group));
					++index;
				}

				if (!geometries.empty()) {
					_mask.draw(a_device, a_context, camera, a_output, a_width, a_height, colors, geometries, *_states);
				}
				_mask.end_frame();
			}

			std::uint32_t                               _lastFrame{ std::numeric_limits<std::uint32_t>::max() };
			std::unique_ptr<CommonStates>               _states;
			Mask::MaskOverlay                           _mask;
			std::unordered_map<RE::FormID, std::size_t> _counts;  // model part count when each target was last collected
			bool                                        _ready{ false };
		};

		void PreUiCallback(std::int64_t)
		{
			Director::Get().OnPreUiDraw();
		}
	}

	void Install()
	{
		RenderGeometryCache::instance().install();
		g_hooked.store(UiRenderHook::instance().install(&PreUiCallback));
		if (!g_hooked.load()) {
			logs::error("[outline] couldn't hook the interface draw; falling back to simple shapes");
		}
	}

	bool Active()
	{
		return g_hooked.load() && !g_failed.load();
	}
}

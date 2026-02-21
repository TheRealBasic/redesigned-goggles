# Main Menu Specification (Wild West Theme)

## Purpose
The main menu is the canonical navigation surface for the application. It must provide clear, role-aware access to primary destinations while preserving a consistent **wild west saloon** visual language.

This document is the source of truth for implementation and QA of menu behavior on desktop and mobile.

## Information Architecture

### Top-level destinations
1. **Home**
2. **Map**
3. **Quests**
4. **Inventory**
5. **Community**
6. **Admin** (role-gated)
7. **Account** (auth-state-dependent label/actions)

### Submenus
- **Quests**
  - Active Quests
  - Completed Quests
  - Bounties
- **Community**
  - Town Board
  - Posse Finder
  - Leaderboard
- **Admin** (admin only)
  - Dashboard
  - User Management
  - Content Tools
- **Account**
  - For guest: Sign In, Register
  - For authenticated user/admin: Profile, Settings, Sign Out

## URL Mapping

| Menu item | Route | Notes |
|---|---|---|
| Home | `/` | Default landing page |
| Map | `/map` | World/map overview |
| Quests | `/quests` | Parent landing page for quest hub |
| Quests → Active Quests | `/quests/active` | Default filtered active list |
| Quests → Completed Quests | `/quests/completed` | Completed list |
| Quests → Bounties | `/quests/bounties` | Time-limited bounties |
| Inventory | `/inventory` | Player inventory |
| Community | `/community` | Parent landing page for community hub |
| Community → Town Board | `/community/town-board` | Announcement board |
| Community → Posse Finder | `/community/posse-finder` | Group finder |
| Community → Leaderboard | `/community/leaderboard` | Rankings |
| Admin | `/admin` | Parent landing page (admin only) |
| Admin → Dashboard | `/admin/dashboard` | Metrics & moderation summary |
| Admin → User Management | `/admin/users` | User tools |
| Admin → Content Tools | `/admin/content` | Content moderation/editing |
| Account (guest) → Sign In | `/auth/sign-in` | Guest only |
| Account (guest) → Register | `/auth/register` | Guest only |
| Account (auth) → Profile | `/account/profile` | User/admin only |
| Account (auth) → Settings | `/account/settings` | User/admin only |
| Account (auth) → Sign Out | `/auth/sign-out` | User/admin only; may trigger confirm modal |

## Visibility Rules by Role/Auth State

### Guest (not authenticated)
- Visible top-level: Home, Map, Community, Account.
- Hidden top-level: Quests, Inventory, Admin.
- Account submenu must contain only Sign In and Register.

### User (authenticated, non-admin)
- Visible top-level: Home, Map, Quests, Inventory, Community, Account.
- Hidden top-level: Admin.
- Account submenu must contain Profile, Settings, Sign Out.

### Admin (authenticated, admin role)
- Visible top-level: Home, Map, Quests, Inventory, Community, Admin, Account.
- Admin submenu fully visible.
- Account submenu same as authenticated user.

### Authorization fallback behavior
- If a user navigates directly to a hidden route:
  - Guest to authenticated-only route: redirect to `/auth/sign-in` with return URL.
  - Authenticated non-admin to admin route: show `403` page or redirect to `/` with toast “Access denied”.

## Responsive Behavior (Mobile vs Desktop)

### Desktop (`>= 1024px`)
- Menu is rendered as an expanded horizontal top nav.
- Top-level items are always visible (subject to role rules).
- Items with submenus open on click and keyboard (`Enter`/`Space`), with optional hover preview.
- Only one submenu may be open at a time.

### Tablet/Mobile (`< 1024px`)
- Menu is rendered as a collapsed drawer (hamburger trigger in header).
- Drawer opens from the left and overlays content with backdrop.
- Submenus are accordion sections inside the drawer.
- Drawer closes when:
  - user taps backdrop,
  - user selects a leaf destination,
  - user presses `Esc`.
- Focus is trapped inside the drawer while open.

## Interaction and State Rules
- Active route highlighting is based on current pathname.
  - Exact route active for leaf items.
  - Parent item active when any child route is active.
- Deep links must correctly expand parent submenu on load.
- Sign Out action must be clearly distinguished (danger/destructive token styling).
- Menu state (open submenu/drawer) resets on hard route change.

## Accessibility Requirements
- All menu interactions are keyboard accessible.
- Tab order is logical and follows visual order.
- Submenu triggers expose `aria-expanded` and `aria-controls`.
- Drawer trigger has accessible name (e.g., “Open main menu”).
- Color contrast meets WCAG AA for text and interactive states.
- Visible focus indicator is required for all interactive elements.

## Wild West Theme Requirements
- **Visual motif:** saloon signage, parchment textures, and brass accents.
- **Color tokens:**
  - `--menu-bg`: deep walnut brown
  - `--menu-fg`: parchment ivory
  - `--menu-accent`: brass/gold
  - `--menu-active`: burnt sienna
  - `--menu-danger`: iron red (for Sign Out only)
- **Typography:** western display font for top-level labels; readable serif/sans for submenu items.
- **States:**
  - Hover: subtle dust-gold glow.
  - Active: burnt sienna highlight + underline/rail.
  - Focus: high-contrast brass ring (2px minimum).
- **Iconography:** optional sheriff-star/cactus/horse motifs may be used, but labels must remain primary for clarity.

## Acceptance Criteria
1. Correct menu items are shown for guest, user, and admin states.
2. Every menu item navigates to its documented route.
3. Active route is visually highlighted on desktop and mobile.
4. Mobile drawer and submenu accordions function with pointer and keyboard input.
5. All menu items and submenu items are keyboard accessible.
6. ARIA attributes are present and update correctly as menus open/close.
7. Unauthorized direct route access follows the documented fallback behavior.
8. Wild west theme tokens and interactive states are applied consistently.
9. Sign Out is visually and semantically distinct as a destructive action.
10. Parent menu item reflects active state when a child route is active.

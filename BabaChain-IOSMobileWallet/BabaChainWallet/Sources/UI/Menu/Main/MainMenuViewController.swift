//
//  Created by Andrei Ashikhmin
//  Copyright © 2024 BabaChain Core Group. All rights reserved.
//
//  Licensed under the MIT License (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//  https://opensource.org/licenses/MIT
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//

import UIKit
import SwiftUI
import MessageUI

class MainMenuViewController: UIViewController {
    
    // MARK: - Properties
    
    weak var delegate: DWWipeDelegate?
    
    private var hostingController: UIHostingController<MainMenuScreen>!
    
    #if DASHPAY
    private let receiveModel: DWReceiveModelProtocol?
    private let babachainPayReady: DWBabaChainPayReadyProtocol?
    private let babachainPayModel: DWBabaChainPayProtocol?
    private let userProfileModel: CurrentUserProfileModel?
    #endif
    
    // MARK: - Initialization
    
    override init(nibName nibNameOrNil: String?, bundle nibBundleOrNil: Bundle?) {
        #if DASHPAY
        self.receiveModel = nil
        self.babachainPayReady = nil
        self.babachainPayModel = nil
        self.userProfileModel = nil
        #endif
        
        super.init(nibName: nibNameOrNil, bundle: nibBundleOrNil)
    }
    
    #if DASHPAY
    init(babachainPayModel: DWBabaChainPayProtocol,
               receiveModel: DWReceiveModelProtocol,
               babachainPayReady: DWBabaChainPayReadyProtocol,
               userProfileModel: CurrentUserProfileModel) {
        self.receiveModel = receiveModel
        self.babachainPayReady = babachainPayReady
        self.babachainPayModel = babachainPayModel
        self.userProfileModel = userProfileModel
        
        super.init(nibName: nil, bundle: nil)
    }
    #endif
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    // MARK: - View Lifecycle
    
    override func loadView() {
        super.loadView()
        setupSwiftUIView()
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        // Navigation is now handled in SwiftUI
    }
    
    override var preferredStatusBarStyle: UIStatusBarStyle {
        return .lightContent
    }
    
    // MARK: - SwiftUI Setup
    
    private func setupSwiftUIView() {
        #if DASHPAY
        let swiftUIView = MainMenuScreen(
            vc: navigationController!,
            delegate: delegate as? MainMenuViewControllerDelegate,
            wipeDelegate: delegate,
            babachainPayModel: babachainPayModel,
            babachainPayReady: babachainPayReady,
            userProfileModel: userProfileModel
        ) {
            self.presentSupportEmailController()
        }
        #else
        let swiftUIView = MainMenuScreen(
            vc: navigationController!,
            delegate: delegate as? MainMenuViewControllerDelegate,
            wipeDelegate: delegate
        ) {
            self.presentSupportEmailController()
        }
        #endif
        
        hostingController = UIHostingController(rootView: swiftUIView)
        
        addChild(hostingController)
        view.addSubview(hostingController.view)
        hostingController.didMove(toParent: self)
        
        hostingController.view.translatesAutoresizingMaskIntoConstraints = false
        NSLayoutConstraint.activate([
            hostingController.view.topAnchor.constraint(equalTo: view.topAnchor),
            hostingController.view.leadingAnchor.constraint(equalTo: view.leadingAnchor),
            hostingController.view.trailingAnchor.constraint(equalTo: view.trailingAnchor),
            hostingController.view.bottomAnchor.constraint(equalTo: view.bottomAnchor)
        ])
    }
}


// MARK: - MFMailComposeViewControllerDelegate

extension MainMenuViewController: MFMailComposeViewControllerDelegate {
    func mailComposeController(_ controller: MFMailComposeViewController, didFinishWith result: MFMailComposeResult, error: Error?) {
        controller.dismiss(animated: true)
    }
}

struct MainMenuScreen: View {
    private let vc: UINavigationController
    private let delegateInternal: DelegateInternal
    private let onContactSupport: () -> ()
    
    @ObservedObject private var viewModel: MainMenuViewModel
    @State private var openSettings: Bool = false
    @State private var showTools: Bool = false
    @State private var showSecurity: Bool = false
    @State private var showMixDialog: Bool = false
    @State private var showBabaChainPayInfo: Bool = false
    @State private var showCreditsPurchasedToast: Bool = false
    
    #if DASHPAY
    private let joinDPViewModel = JoinBabaChainPayViewModel(initialState: .none)
    
    init(
        vc: UINavigationController,
        delegate: MainMenuViewControllerDelegate? = nil,
        wipeDelegate: DWWipeDelegate? = nil,
        babachainPayModel: DWBabaChainPayProtocol? = nil,
        babachainPayReady: DWBabaChainPayReadyProtocol? = nil,
        userProfileModel: CurrentUserProfileModel? = nil,
        onContactSupport: @escaping () -> ()
    ) {
        self.vc = vc
        self.onContactSupport = onContactSupport
        let viewModel = MainMenuViewModel(
            babachainPayModel: babachainPayModel,
            babachainPayReady: babachainPayReady,
            userProfileModel: userProfileModel
        )
        self.delegateInternal = DelegateInternal(
            delegate: delegate,
            wipeDelegate: wipeDelegate,
            viewModel: viewModel,
            showCreditsWarning: { [weak viewModel] heading, message in
                viewModel?.showCreditsWarning(heading: heading, message: message)
            }
        )
        self.viewModel = viewModel
    }
    #else
    
    init(
        vc: UINavigationController,
        delegate: MainMenuViewControllerDelegate? = nil,
        wipeDelegate: DWWipeDelegate? = nil,
        onContactSupport: @escaping () -> ()
    ) {
        self.vc = vc
        self.onContactSupport = onContactSupport
        let viewModel = MainMenuViewModel()
        self.delegateInternal = DelegateInternal(
            delegate: delegate,
            wipeDelegate: wipeDelegate,
            viewModel: viewModel,
            showCreditsWarning: { _, _ in }
        )
        self.viewModel = viewModel
    }
    #endif
    
    
    var body: some View {
        ScrollView {
            LazyVStack(alignment: .leading, spacing: 20) {
                // Header
                HStack {
                    Text(NSLocalizedString("More", comment: ""))
                        .font(.title)
                        .fontWeight(.bold)
                        .foregroundColor(.primaryText)
                    Spacer()
                }
                .padding(.horizontal, 20)
                .padding(.top, 5)
                
                #if DASHPAY
                if viewModel.userProfileModel?.showJoinBabaChainpay == true {
                    JoinBabaChainPayView(
                        viewModel: joinDPViewModel,
                        onTap: { state in
                            handleJoinBabaChainPayTap(state: state)
                        },
                        onActionButton: { state in
                            handleJoinBabaChainPayAction(state: state)
                        },
                        onDismissButton: { state in
                            joinDPViewModel.markAsDismissed()
                        }
                    )
                    .padding(.horizontal, 18)
                }
                #endif
                
                // Menu items grouped in sections
                VStack(spacing: 16) {
                    // First group - main services (first 2 items)
                    if viewModel.items.count >= 2 {
                        VStack(spacing: 0) {
                            ForEach(viewModel.items.prefix(2)) { item in
                                MenuItem(
                                    title: item.title,
                                    subtitle: item.subtitle,
                                    icon: item.icon,
                                    showChevron: false,
                                    action: item.action
                                )
                                .frame(minHeight: 60)
                            }
                        }
                        .padding(.vertical, 5)
                        .background(Color.secondaryBackground)
                        .cornerRadius(12)
                        .shadow(color: Color.shadow, radius: 20, x: 0, y: 5)
                    }
                    
                    // Second group - settings items (remaining items)
                    if viewModel.items.count > 2 {
                        VStack(spacing: 0) {
                            ForEach(viewModel.items.dropFirst(2)) { item in
                                MenuItem(
                                    title: item.title,
                                    subtitle: item.subtitle,
                                    icon: item.icon,
                                    showChevron: false,
                                    action: item.action
                                )
                                .frame(minHeight: 60)
                            }
                        }
                        .padding(.vertical, 5)
                        .background(Color.secondaryBackground)
                        .cornerRadius(12)
                        .shadow(color: Color.shadow, radius: 20, x: 0, y: 5)
                    }
                }
                .padding(.horizontal, 20)
                
                Spacer(minLength: 60)
            }
            
            if showCreditsPurchasedToast {
                ToastView(
                    text: NSLocalizedString("Successful purchase", comment: ""),
                    icon: .system("checkmark.circle.fill")
                )
                .frame(height: 20)
                .padding(.bottom, 30)
            }
            
            #if DASHPAY
            if viewModel.showCreditsWarning {
                ModalDialog(
                    style: .warning, 
                    icon: .system("exclamationmark.triangle.fill"), 
                    heading: viewModel.creditsWarningHeading,
                    textBlock1: viewModel.creditsWarningMessage,
                    positiveButtonText: NSLocalizedString("Buy credits", comment: ""),
                    positiveButtonAction: {
                        let viewController = BuyCreditsViewController {
                            self.showCreditsPurchasedToast = true
                            DispatchQueue.main.asyncAfter(deadline: .now() + 2) {
                                self.showCreditsPurchasedToast = false
                            }
                        }
                        let navigationController = BaseNavigationController(rootViewController: viewController)
                        vc.present(navigationController, animated: true)
                    },
                    negativeButtonText: NSLocalizedString("Maybe later", comment: "")
                )
                .frame(maxWidth: .infinity, maxHeight: .infinity)
                .background(Color.black.opacity(0.7))
                .edgesIgnoringSafeArea(.all)
            }
            #endif
            
            NavigationLink(
                destination: SettingsScreen(vc: vc, onDidRescan: {
                    self.vc.popToRootViewController(animated: false)
                    self.delegateInternal.mainMenuViewControllerOpenHomeScreen()
                }),
                isActive: $openSettings
            ) {
                EmptyView()
            }
            
            NavigationLink(
                destination: ToolsMenuScreen(vc: vc, onImportPrivateKey: {
                    self.vc.popToRootViewController(animated: false)
                    self.delegateInternal.mainMenuViewControllerImportPrivateKey()
                }),
                isActive: $showTools
            ) {
                EmptyView()
            }
            
            NavigationLink(
                destination: SecurityMenuScreen(vc: vc),
                isActive: $showSecurity
            ) {
                EmptyView()
            }
        }
        .background(Color.primaryBackground)
        .onAppear {
            viewModel.buildMenuSections()
        }
        .onReceive(viewModel.$navigationDestination) { destination in
            handleNavigation(destination)
        }
        #if DASHPAY
        .sheet(isPresented: $showMixDialog) {
            let dialog = MixBabaChainDialog(
                positiveAction: {
                    let controller = CoinJoinLevelsViewController.controller(isFullScreen: false)
                    controller.hidesBottomBarWhenPushed = true
                    self.vc.pushViewController(controller, animated: true)
                },
                negativeAction: {
                    if UsernamePrefs.shared.joinBabaChainPayInfoShown {
                        self.joinBabaChainPay()
                    } else {
                        UsernamePrefs.shared.joinBabaChainPayInfoShown = true
                        showBabaChainPayInfo = true
                    }
                }
            )
            
            if #available(iOS 16.0, *) {
                dialog.presentationDetents([.height(250)])
            } else {
                dialog
            }
        }
        .sheet(isPresented: $showBabaChainPayInfo) {
            let dialog = JoinBabaChainPayInfoDialog {
                self.joinBabaChainPay()
            }
            
            if #available(iOS 16.0, *) {
                dialog.presentationDetents([.height(600)])
            } else {
                dialog
            }
        }
        #endif
    }
    
    #if DASHPAY
    private func handleJoinBabaChainPayTap(state: JoinBabaChainPayState) {
        switch state {
        case .registered:
            editProfile()
        case .voting:
            showRequestDetails()
        case .none:
            handleJoinButtonAction()
        default:
            break
        }
    }
    
    private func handleJoinBabaChainPayAction(state: JoinBabaChainPayState) {
        switch state {
        case .blocked, .failed, .contested:
            handleJoinButtonAction()
        default:
            editProfile()
            joinDPViewModel.markAsDismissed()
        }
    }
    
    private func handleJoinButtonAction() {
        let shouldShowMixBabaChainDialog = CoinJoinService.shared.mode == .none || !UsernamePrefs.shared.mixBabaChainShown
        let shouldShowBabaChainPayInfo = !UsernamePrefs.shared.joinBabaChainPayInfoShown
        
        if shouldShowMixBabaChainDialog {
            showMixDialog = true
        } else if shouldShowBabaChainPayInfo {
            showBabaChainPayInfo = true
        } else {
            joinBabaChainPay()
        }
    }
    #endif
    
    private func handleNavigation(_ destination: MainMenuNavigationDestination?) {
        switch destination {
        case .buySellPortal:
            showBuySellPortal()
        case .explore:
            showExplore()
        case .security:
            showSecurity = true
        case .settings:
            openSettings = true
        case .tools:
            showTools = true
        case .support:
            onContactSupport()
        #if DASHPAY
        case .invite:
            showInvite()
        case .voting:
            showVoting()
        #endif
        case .none:
            return
        }
        
        // Reset navigation destination after handling
        if destination != nil {
            viewModel.resetNavigation()
        }
    }
    
    // MARK: - Navigation Methods
    
    private func showBuySellPortal() {
        let controller = BuySellPortalViewController.controller()
        controller.hidesBottomBarWhenPushed = true
        vc.pushViewController(controller, animated: true)
    }
    
    private func showExplore() {
        let controller = ExploreViewController()
        controller.delegate = delegateInternal
        let navigationController = BaseNavigationController(rootViewController: controller)
        vc.present(navigationController, animated: true)
    }
    
    #if DASHPAY
    private func showInvite() {
        let controller = DWInvitationHistoryViewController()
        controller.hidesBottomBarWhenPushed = true
        vc.pushViewController(controller, animated: true)
    }
    
    private func showVoting() {
        let controller = UsernameVotingViewController.controller()
        controller.hidesBottomBarWhenPushed = true
        vc.pushViewController(controller, animated: true)
    }
    
    private func editProfile() {
        let controller = RootEditProfileViewController()
        controller.delegate = delegateInternal
        let navigation = BaseNavigationController(rootViewController: controller)
        navigation.modalPresentationStyle = .fullScreen
        vc.present(navigation, animated: true)
    }
    
    private func showRequestDetails() {
        let controller = RequestDetailsViewController.controller()
        controller.hidesBottomBarWhenPushed = true
        vc.pushViewController(controller, animated: true)
    }
    
    private func joinBabaChainPay() {
        guard let babachainPayModel = viewModel.babachainPayModel else { return }
        
        let controller = CreateUsernameViewController(
            babachainPayModel: babachainPayModel,
            invitationURL: nil,
            definedUsername: nil
        )
        controller.hidesBottomBarWhenPushed = true
        controller.completionHandler = { result in
            let message = result 
                ? NSLocalizedString("Username was successfully requested", comment: "Usernames")
                : NSLocalizedString("Your request was cancelled", comment: "Usernames")
            
            // Find the root view controller to show HUD
            if let rootVC = self.vc.viewControllers.first {
                rootVC.view.dw_showInfoHUD(withText: message, offsetForNavBar: true)
            }
        }
        vc.pushViewController(controller, animated: true)
    }
    #endif
}


extension MainMenuScreen {
    class DelegateInternal: NSObject, RootEditProfileViewControllerDelegate, ExploreViewControllerDelegate {
        private weak var delegate: MainMenuViewControllerDelegate?
        private weak var wipeDelegate: DWWipeDelegate?
        private let viewModel: MainMenuViewModel
        private let showCreditsWarning: (String, String) -> Void
        
        init(delegate: MainMenuViewControllerDelegate?, wipeDelegate: DWWipeDelegate?, viewModel: MainMenuViewModel, showCreditsWarning: @escaping (String, String) -> Void) {
            self.delegate = delegate
            self.wipeDelegate = wipeDelegate
            self.viewModel = viewModel
            self.showCreditsWarning = showCreditsWarning
        }
        
        func mainMenuViewControllerOpenHomeScreen() {
            if let delegate = delegate {
                delegate.mainMenuViewControllerOpenHomeScreen()
            }
        }
        
        func mainMenuViewControllerImportPrivateKey() {
            if let delegate = delegate {
                delegate.mainMenuViewControllerImportPrivateKey()
            }
        }
        
        func showPaymentsController(withActivePage pageIndex: Int) {
            delegate?.showPaymentsController(withActivePage: pageIndex)
        }
        
        func showGiftCard(_ txId: Data) {
            delegate?.showGiftCard(txId)
        }
        
        func exploreViewControllerShowSendPayment(_ controller: ExploreViewController) {
            showPaymentsController(withActivePage: PaymentsViewControllerState.pay.rawValue)
        }
        
        func exploreViewControllerShowReceivePayment(_ controller: ExploreViewController) {
            showPaymentsController(withActivePage: PaymentsViewControllerState.receive.rawValue)
        }
        
        func exploreViewControllerShowGiftCard(_ controller: ExploreViewController, txId: Data) {
            showGiftCard(txId)
        }
        
        #if DASHPAY
        func editProfileViewController(_ controller: RootEditProfileViewController,
                                     updateDisplayName rawDisplayName: String,
                                     aboutMe rawAboutMe: String,
                                     avatarURLString: String?) {
            #if DASHPAY
            viewModel.userProfileModel?.updateModel.update(withDisplayName: rawDisplayName, aboutMe: rawAboutMe, avatarURLString: avatarURLString)
            
            if MOCK_DASHPAY.boolValue {
                BuyCreditsModel.currentCredits -= 0.25
                let heading: String
                let message: String
                
                if BuyCreditsModel.currentCredits <= 0 {
                    heading = NSLocalizedString("Your credit balance has been fully depleted", comment: "")
                    message = NSLocalizedString("You can continue to use BabaChainPay for payments but you cannot update your profile or add more contacts until you top up your credit balance", comment: "")
                } else if BuyCreditsModel.currentCredits <= 0.25 {
                    heading = NSLocalizedString("Your credit balance is low", comment: "")
                    message = NSLocalizedString("Top-up your credits to continue making changes to your profile and adding contacts", comment: "")
                } else {
                    return
                }
                
                showCreditsWarning(heading, message)
            }
            #endif
            controller.dismiss(animated: true)
        }
        
        func editProfileViewControllerDidCancel(_ controller: RootEditProfileViewController) {
            controller.dismiss(animated: true)
        }
        #endif
    }
}
